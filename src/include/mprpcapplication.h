#pragma once

#include "mprpcconfig.h"

// mprpc框架的基础类，设计为单例模式
class MprpcApplication
{
public:
    static void Init(int argc, char** argv);
    static MprpcApplication& GetInstance();

private:
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;

    static MprpcConfig m_config;
};