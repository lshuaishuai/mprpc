#include <iostream>

#include "user.pb.h"
#include "mprpcapplication.h"


int main(int argc, char** argv)
{
    // 整个程序启动以后，想使用mprpc框架享受rpc服务调用，一定需要调用框架的初始化函数(只初始化一次)
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login 想要实例化Stub必须有一个Channel
    fixbug::UserServiceRPC_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("shuaishuai");
    request.set_pwd("195370");

    // rpc方法的响应
    fixbug::LoginResponse response;
    // rpc方法的调用
    stub.Login(nullptr, &request, &response, nullptr);    // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    if(0 == response.result().errcode())
    {
        std::cout << "rpc login response success: " << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
    }

    // 演示调用远程发布的rpc方法 register
    fixbug::RegisterRequest req;
    req.set_name("tingting");
    req.set_id(2000);
    req.set_pwd("195370");
    fixbug::RegisterResponse rsp;
    
    // 以同步的方式发起rpc请求，等待返回结果
    stub.Register(nullptr, &req, &rsp, nullptr);

    if(0 == rsp.result().errcode())
    {
        std::cout << "rpc register response success: " << rsp.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error: " << rsp.result().errmsg() << std::endl;
    }
    
    return 0;
}