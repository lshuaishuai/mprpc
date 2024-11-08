#include <iostream>
#include <string>

#include "../user.pb.h"
#include "../../src/include/mprpcapplication.h"
#include "../../src/include/rpcprovider.h"

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/
class UserService: public fixbug::UserServiceRPC  // 使用在rpc服务提供方/发布端
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;

        return false;
    }

    /*
    重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
    1. caller  ==>   Login(LoginRequest)  =>  nuduo  =>  callee
    2. callee  ==>   Login(LoginRequest)  =>  交到下面重写的Login中
    */
    // 我们现在所处的是callee(服务提供方)的视角，远端发来的RPC请求首先被RPC框架接收 匹配到对应的方法 从请求中拿出数据，执行本地业务，填充响应，执行回调由框架返回响应
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数LoginRequest，业务获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool loginresult = Login(name, pwd);   

        // 把响应写入 包括错误码 错误消息
        response->set_success(loginresult);
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");

        // 执行回调操作: 执行响应对象数据的序列化和网络发送(都是由框架来完成的)
        done->Run();
    }
};

int main(int argc, char** argv)
{
    // 调用框架的初始化操作
    // provider -i config.conf
    MprpcApplication::Init(argc, argv);

    // RpcProvider是一个rpc网络服务对象，发布服务的对象
    RpcProvider provider;
    // 把UserService对象发送到rpc节点上
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点 Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}