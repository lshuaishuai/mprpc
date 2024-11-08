#pragma once

#include "google/protobuf/service.h"

// 框架提供的专门发布rpc服务的网络对象类
class RpcProvider
{
public:
    // 由proto中service方法生成的类都是继承自Service的，所以这里用Service指针即可(多态)
    // 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service* service);

    // 启动rpc服务节点，开始提供rpc远程调用服务
    void Run();
};