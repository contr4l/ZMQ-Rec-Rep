#ifndef _PARSE_CFG_H_
#define _PARSE_CFG_H_

#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <chrono>
#include <iomanip>
#include <vector>

std::unordered_map<std::string, std::string> parse_cfg(int argc, char* argv[]);

std::string getCurrentDateTimeString();
uint32_t ipStringToUint32(std::string& ip);
std::string ipPortToTcpString(uint32_t ip, uint16_t port);
uint32_t monotonic_timestmap();

#endif