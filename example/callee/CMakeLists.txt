set(SRC_LIST userservice.cc ../user.pb.cc)

# 定义了一个可执行文件 provider，它由 SRC_LIST 中的源文件编译而成
add_executable(provider ${SRC_LIST})
# 将 mprpc 和 protobuf 库链接到 provider 可执行文件
target_link_libraries(provider mprpc protobuf)