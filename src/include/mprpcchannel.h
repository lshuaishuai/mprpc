#pragma once

#include <google/protobuf/service.h>

class MprpcChannel: public google::protobuf::RpcChannel
{
public:
    MprpcChannel(){} 
    // 他是如何被调用的呢？ -- 客户端实例化Stub对象 然后stub.Login(); Login里面就会调用CallMethod。不只是Login只要是被发布为rpc的服务都是调用这个函数
    // 所有通过stub代理对象调用的rpc方法，都走到了这儿，统一做rpc方法调用的数据序列化和网络发送
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, 
                    const google::protobuf::Message* request,
                          google::protobuf::Message* response, 
                          google::protobuf::Closure* done);
private:

};