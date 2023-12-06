#ifndef UTILS_HH
#define UTILS_HH

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>

template <typename... Args>
void Log(Args... args)
{
    std::ostringstream oss;
    ((oss << args), ...);
    std::string message = oss.str();

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    // 格式化时间戳为 "yyyy-mm-dd-hh:mm:ss" 格式
    std::tm timeInfo = *std::localtime(&currentTime);
    std::ostringstream timestamp;
    timestamp << std::put_time(&timeInfo, "[%Y-%m-%d %H:%M:%S]");

    std::cout << timestamp.str() << " " << message << std::endl;
}

#endif // UTILS_HH
