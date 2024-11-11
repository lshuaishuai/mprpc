#include <iostream>

#include "friend.pb.h"
#include "mprpcapplication.h"
#include "logger.h"


int main(int argc, char** argv)
{
    // 整个程序启动以后，想使用mprpc框架享受rpc服务调用，一定需要调用框架的初始化函数(只初始化一次)
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login 想要实例化Stub必须有一个Channel
    fixbug::FriendServiceRPC_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::GetFriendsRequest request;
    request.set_userid(1000);

    // rpc方法的响应
    fixbug::GetFriendsResponse response;
    // rpc方法的调用
    MprpcController controller;
    stub.GetFriendsList(&controller, &request, &response, nullptr);    // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if(0 == response.result().errcode())
        {
            int size = response.friends_size();
            for(int i = 0; i < size; i++)
            {
                std::cout << "index: " << (i+1) << " name: " << response.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendsList response error: " << response.result().errmsg() << std::endl;
        }
    }
    return 0;
}