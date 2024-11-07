#include "test.pb.h"
#include <iostream>
#include <string>

using namespace fixbug;

int main()
{
    // LoginResponse rsp;
    // ResultCode* rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("登陆处理失败");

    GetFriendListsResponse rsp;
    ResultCode* rc = rsp.mutable_result();
    rc->set_errcode(0);

    User* us1 = rsp.add_friend_list();  // add就是往列表中添加元素
    us1->set_name("zhangsan");
    us1->set_age(20);
    us1->set_sex(User::MAN);
    std::cout << "friend_list_size: " << rsp.friend_list_size() << std::endl;
    
    User* us2 = rsp.add_friend_list();
    us2->set_name("里斯");
    us2->set_age(21);
    us2->set_sex(User::WOMAN);
    std::cout << "friend_list_size: " << rsp.friend_list_size() << std::endl;

    return 0;
}

int a()
{
    // 封装了login请求对象的数据
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");

    // 对象数据序列化 => char*
    std::string send_str;
    if(req.SerializeToString(&send_str))
    {
        std::cout << send_str << std::endl;
    }

    // 反序列化一个login请求对象
    LoginRequest reqB;
    if(reqB.ParseFromString(send_str))
    {
        std::cout << reqB.name() << std::endl;
        std::cout << reqB.pwd() << std::endl;
    }

    LoginResponse resp;

    return 0;
}