# 将所有当前目录下的源文件.cpp文件添加到变量中
# aux_source_directory(. SRC_LIST)
set(SRC_LIST 
    mprpcapplication.cc 
    mprpcconfig.cc 
    mprpcprovider.cc 
    rpcheader.pb.cc 
    mprpcchannel.cc
    mprpccontroller.cc
    logger.cc
    zookeeperutil.cc)
# 使用 SRC_LIST 中的源文件创建一个名为 mprpc 的共享库 .so
add_library(mprpc ${SRC_LIST})
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)