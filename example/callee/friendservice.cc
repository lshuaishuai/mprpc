#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"
#include "logger.h"
#include <vector>
#include <memory>

class FriendService: public fixbug::FriendServiceRPC
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service! userid = " << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("baba");
        vec.push_back("mama");
        vec.push_back("jiejie");
        return vec;
    }

    void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsRequest* request,
                       ::fixbug::GetFriendsResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t userid = request->userid();

        std::vector<std::string> friendsList = GetFriendsList(userid);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");

        for(std::string& name : friendsList)
        {
            std::string* p = response->add_friends();
            *p = name;
        }

        done->Run();
    }
};

int main(int argc, char** argv)
{
    // LOG_INFO("first log message!"); 
    // LOG_ERR("%s : %s : %d", __FILE__, __FUNCTION__, __LINE__);  
    MprpcApplication::GetInstance().Init(argc, argv);

    std::shared_ptr<RpcProvider> provider(new RpcProvider());
    provider->NotifyService(new FriendService());

    provider->Run();

    return 0;
}