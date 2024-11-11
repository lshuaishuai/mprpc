#include "zookeeperutil.h"
#include "mprpcapplication.h"

// 全局的watcher观察器  zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
                    int state, const char *path,void *watcherCtx)
{
    if(type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
    {
        if(state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
        {
            sem_t* sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient()
    :m_zhandle(nullptr)
{}

ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr) zookeeper_close(m_zhandle);   // 关闭句柄 释放资源
}

// zkclient启动连接zkserver
void ZkClient::start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    /*
    zookeeper_mt: 多线程版本
    zookeeper的API客户端程序提供了三个线程
    API调用线程
    网络IO线程 pthread_create poll
    watcher回调线程 pthread_create
    */ 
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if(nullptr == m_zhandle)
    {
        // std::cout << "zookeeper_init error!" << std::endl;
        LOG_ERR("zookeeper_init error!");
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);  // 一开始sem=0会阻塞到这里，直到连接上zkserver(根据回调函数来确定)
    // std::cout << "zookeeper_init success!" << std::endl;
    LOG_INFO("zookeeper_init success!");
}

// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char* path, const char* data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    flag = zoo_exists(m_zhandle, path, 0, nullptr);

    // 先判断path表示的节点是否存在 如果存在就不再重复创建了
    if (flag == ZNONODE) 
    {
        // 创建指定path的znode节点
        int rc = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (rc == ZOK) {
            // std::cerr << "Failed to initiate async create node!" << std::endl;
            LOG_ERR("znode create success... path: %s", path);
        }
        else {
            // std::cerr << "znode create error... path: " << path << ", error: " << zerror(create_result_code) << std::endl;
            LOG_ERR("flag: %d", flag);
            LOG_ERR("znode create error... path: %s", path);
            exit(EXIT_FAILURE);
        }
    } 
}

// 更具参数指定的znode主节点路径，获取节点的值
std::string ZkClient::GetData(const char* path)
{
    char buffer[64];
    int bufferlen = sizeof(buffer);
    // 使用异步的 zoo_aget 获取节点数据
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK) {
        LOG_ERR("get znode error... path: %s", path);
        return "";
    }
    else return buffer;
}