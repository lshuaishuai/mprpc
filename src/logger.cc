#include "logger.h"

#include <thread>
#include <iostream>
#include <time.h>

// 获取日志的单例
Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    // 启动专门写日志的线程  将日志从队列写入到磁盘
    std::thread writeLogTask([&](){
        for(;;)
        {
            // 获取当天的日期，然后获取日志信息，写入相应的日志文件当中 a+
            time_t now = time(nullptr);
            tm* now_tm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "logs/%.4d-%.2d-%.2d-log.txt", now_tm->tm_year+1900, now_tm->tm_mon+1, now_tm->tm_mday);

            FILE* pf = fopen(file_name, "a+");
            if(nullptr == pf)
            {
                std::cout << "logger file: " << file_name << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lockQueue.Pop();
            char time_buf[128] = {0};
            sprintf(time_buf, "%.2d:%.2d:%.2d => [%s] ", now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec, (m_loglevel == INFO ? "info" : "error"));
            msg.insert(0, time_buf);
            msg.append("\n");
            fputs(msg.c_str(), pf);
            fclose(pf);
        }
    });
    // 设置分离线程，守护线程
    writeLogTask.detach();
}

// 设置日志的级别
void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel = level;
}

// 写日志 将日志队列写入lockqueue缓冲区中
void Logger::Log(const std::string& msg)
{
    m_lockQueue.Push(msg);
}