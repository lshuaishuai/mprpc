#include "include/mprpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

/*
service_name => service描述
                       => service* 记录服务对象
                       method_name  =>  method方法对象
*/
// 发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service)
{
    ServiceInfo service_info;
    // 获取了服务对象的描述信息 例如example里面的 UserService类对象和其方法的描述
    const google::protobuf::ServiceDescriptor* pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法数量
    int methodCnt = pserviceDesc->method_count();

    // std::cout << "service_name: " << service_name << std::endl;  
    LOG_INFO("service_name: %s", service_name.c_str());

    for(int i = 0; i < methodCnt; ++i)
    {
        // 获取了读物对象指定下标的服务方法的描述(抽象描述)
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();  // 若是rpc请求过来，就会根据匹配对应的name去调用对应的方法
        service_info.m_methodMap.insert({method_name, pmethodDesc});
        // std::cout << "method_name: " << method_name << std::endl;  
        LOG_INFO("method_name: %s", method_name.c_str());
    }
    service_info.m_service = service;
    // std::cout << "======================" << std::endl;
    // ServiceInfo empty_service_info;
    // m_serviceMap["test_service"] = empty_service_info;  // 插入初始化的 ServiceInfo

    // std::cout << "======================" << std::endl;
    m_serviceMap.insert({service_name, service_info});

}

// 启动rpc服务节点，开始提供rpc远程调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);
    
    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定链接回调和消息读写回调方法，分离了网络代码和业务代码
    // 有连接上来，muduo会调用这个回调OnConnection
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    // 有请求发来，muduo'库会调用这个函数OnMessage
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 把当前节点上要发布的服务全部注册到zk上面  让rpcclient可以从zk上发现服务
    // session timeout 30s     zkclient 网络IO线程 1/3*timeout 时间发送ping消息--心跳报文
    ZkClient zkCli;
    zkCli.start();
    for(auto& sp : m_serviceMap)
    {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for(auto& mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示是一个临时节点，某一个服务停止的话，方法也应该就没有了
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout << "RpcProvider start service at ip: " << ip << " port: " << port << std::endl;

    // 启动网络服务器
    server.start();
    // 当有事件就绪的时候，木朵库会自己调用上面绑定好的回调函数
    m_eventLoop.loop();
}

// 新的socket的连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    // 短链接
    if(!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();   // 关闭socket文件描述符
    }
}

/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name和method_name args(参数) 定义proto的message类型，进行数据头的序列化和反序列化
                                     数据头：service_name method_name args_size(防止粘包)
header_size(4字节) + header_str + args_str 
注意在存储header_size时，并不是将header_size直接转为字符串形式，而是二进制 前四个字节只存header_size
*/
// 已建立连接用户的读写事件回调 如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应 负责做序列化和反序列化
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp)
{
    // 此时muduo库调用这个方法后已经将读到的数据放到buffer中了
    // 这里就相当于是Reactor框架中对读到的消息进行处理，在那儿就是相当于业务层
    // 网络上接收的远程rpc调用请求的字符流  Login args
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    // 将recv_buf里面的内容从0开始拷贝4个字节到header_size中
    recv_buf.copy((char*)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字节流  反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    // 将rpcHeader也弄成结构化的数据，但是这里不是自己弄，而是由protobuf
    // rpc_header_str 是由 Protobuf 格式化的二进制数据生成的，
    // 而 ParseFromString 是 Protobuf 提供的反序列化方法，专门用于将二进制字符串转换为 Protobuf 定义的结构化数据
    // rpc_header_str就是由protobuf序列化来的，所以反序列化也是按照protobuf的协议进行的，不会出错
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败 接收了无效的数据
        // std::cout << "rpc_header_str: " << rpc_header_str << " parse error!" << std::endl;
        LOG_ERR("rpc_header_str: %s parse error!", rpc_header_str);
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4+header_size, args_size);

    // 打印调试信息
    std::cout << "======================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "======================" << std::endl;

    // 获取service对象和method方法
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end())
    {
        // std::cout << service_name << " is not exist!" << std::endl;
        LOG_ERR("%s is not exist!", service_name);
        return;
    }
    
    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        // std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        LOG_ERR("%s : %s is not exist!", service_name, method_name);
        return;
    }

    google::protobuf::Service* service = it->second.m_service;        // 获取service对象  new UserService
    const google::protobuf::MethodDescriptor* method = mit->second;   // 获取method方法   Login

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        // std::cout << "request parse error! content: " << args_str << std::endl;
        LOG_ERR("request parse error! content: %s", args_str);
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    // NewCallback 能够创建一个带有绑定参数的 Closure 对象
    // 当 Run() 被调用时，它会自动调用绑定的函数，并传入之前绑定的参数。这是 NewCallback 的一个特性
    google::protobuf::Closure* done = 
                google::protobuf::NewCallback<RpcProvider, const muduo::net::TcpConnectionPtr&, google::protobuf::Message*>
                                             (this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的服务
    // new UserService().Login(controller, request, response, done)
    // 这里的response会将
    service->CallMethod(method, nullptr, request, response, done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response)
{
    // response序列化
    std::string response_str;
    if(response->SerializeToString(&response_str))
        // 序列化成功后，通过网络把rpc的响应发送给调用方
        conn->send(response_str);
    else
    {
        // std::cout << "serialize response_str error!" << std::endl;
        LOG_ERR("serialize response_str error!");
    }    
    conn->shutdown();  // 模拟http的短链接服务，有rocprovider主动断开连接
}