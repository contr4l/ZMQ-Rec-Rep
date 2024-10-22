#include "parse_cfg.h"
#include <cstdint>

std::unordered_map<std::string, std::string> parse_cfg(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.substr(0, 2) == "--") {
            std::string key = arg.substr(2);

            // 检查是否有对应的值
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                std::string value = argv[i + 1];
                args[key] = value;
                ++i;
            }
            else {
                // 如果没有值，可以设置一个默认值或抛出错误
                throw std::invalid_argument("Missing value for argument: " + key);
            }
        }
    }

    return args;
}

std::string getCurrentDateTimeString() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 将时间点转换为time_t
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // 将time_t转换为tm结构
    std::tm now_tm = *std::localtime(&now_time_t);

    // 创建一个字符串流
    std::ostringstream oss;

    // 设置输出格式
    oss << std::put_time(&now_tm, "%Y-%m-%d_%H-%M-%S");

    // 返回字符串
    return oss.str();
}

uint32_t monotonic_timestmap() {
    struct timespec start_mono_time;
    clock_gettime(CLOCK_MONOTONIC, &start_mono_time);
    return start_mono_time.tv_sec * 1000 + start_mono_time.tv_nsec / 1000000;
}

uint32_t ipStringToUint32(std::string& ip) {
    std::vector<int> octets;
    std::istringstream iss(ip);
    std::string octet;

    // 分割IP字符串
    while (std::getline(iss, octet, '.')) {
        octets.push_back(std::stoi(octet));
    }

    // 检查是否分割出4个部分
    if (octets.size() != 4) {
        throw std::invalid_argument("Invalid IP address format");
    }

    // 将4个部分组合成一个uint32_t
    uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        result = (result << 8) | octets[i];
    }

    return result;
}

std::string ipPortToTcpString(uint32_t ip, uint16_t port) {
    std::ostringstream oss;
    oss << "tcp://";
    oss << ((ip >> 24) & 0xFF) << ".";
    oss << ((ip >> 16) & 0xFF) << ".";
    oss << ((ip >> 8) & 0xFF) << ".";
    oss << (ip & 0xFF) << ":";
    oss << port;
    return oss.str();
}