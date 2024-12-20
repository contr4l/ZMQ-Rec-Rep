#ifndef _ALGO_RECORDER_H_
#define _ALGO_RECORDER_H_

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <math.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "zmq.hpp"
#include "parse_cfg.h"

constexpr int ms2s = 1000;
constexpr int ns2s = 1000000000;
constexpr int write_buf_len = 65535;

class Recorder {
    // 接收zmq感知结果，发送至DDS
private:
    int _active_flag = 1;
    std::thread receive_thread;

private:
    zmq::context_t _context;
    std::vector<zmq::socket_t> _sockets;
    std::vector<zmq::pollitem_t> poll_items;

    std::map<zmq::socket_ref, std::pair<int, int>> _socket_src_map;

    std::ofstream outFile;

    int writeFile(const char* data, int size, bool instant = false) {
        if (size > write_buf_len) {
            outFile.close();
            throw std::runtime_error("zmq write buffer too long");
        }

        std::string filename = "zmq_rec_" + getCurrentDateTimeString() + ".bin";
        if (!outFile.is_open()) {
            outFile.open(filename, std::ios::binary | std::ios::out);
        }

        static char buf[write_buf_len] = {0};
        static int pos = 0;
        // buffer write, 降低写入频率

        int write_len = 0;
        if (instant) {
            // 立即写入
            write_len = size + pos;
            if (pos > 0) {
                outFile.write(buf, pos);
            }
            if (data && size) {
                outFile.write(data, size);
            }
            pos = size = 0;
            spdlog::info("instant write to file for {} bytes.", write_len);
        }
        else if (pos + size > write_buf_len) {
            // 缓存写入
            spdlog::info("cache write to file for {} bytes.", pos);
            outFile.write(buf, pos);
            write_len += pos;
            pos = 0;
        }

        memcpy(buf + pos, data, size);
        pos += size;
        // spdlog::info("cache size inc to {}", pos);
        return 0;
    }

    void init_recv_socket(zmq::socket_t& sock, std::string uri, std::string filter, std::string mode) {
        printf("init recv socket %s\n", uri.c_str());
        sock.connect(uri);
        sock.set(zmq::sockopt::subscribe, filter);
        poll_items.push_back({sock, 0, ZMQ_POLLIN, 0});
    }

    void msg_record(zmq::message_t& msg) {
        const char* data = static_cast<const char*>(msg.data());
        int size = msg.size();
        writeFile(data, size);
    }

    void msg_record(zmq::socket_ref sock) {
        if (_socket_src_map.find(sock) == _socket_src_map.end()) {
            printf("socket ref not found\n");
            return;
        }
        auto hosts = _socket_src_map[sock];
        auto timestamp = monotonic_timestmap();
        writeFile((char*)&timestamp, sizeof(uint32_t));
        writeFile((char*)&hosts.first, sizeof(uint32_t));
        writeFile((char*)&hosts.second, sizeof(uint16_t));
    }

    void msg_record(uint16_t length) {
        writeFile((const char*)&length, sizeof(uint16_t));
    }

    void receive_process() {
        const std::chrono::milliseconds timeout{1000};

        while (_active_flag) {
            zmq::poll(poll_items.data(), poll_items.size(), timeout);

            for (int ind = 0; ind < poll_items.size(); ++ind) {
                if (!(poll_items[ind].revents & ZMQ_POLLIN)) {
                    continue;
                }
                zmq::socket_ref sock = _sockets[ind];
                // 写入当前socket信息
                msg_record(sock);

                zmq::message_t topic, body;
                auto res = sock.recv(topic, zmq::recv_flags::none);
                if (topic.more()) {
                    // 有topic的情况
                    spdlog::info("receive msg on {}, topic is {}", ind, topic.to_string());
                    sock.recv(body, zmq::recv_flags::none);
                    msg_record(topic.size());
                    msg_record(body.size());
                    msg_record(topic);
                    msg_record(body);
                }
                else {
                    // 没有topic的情况
                    msg_record(0);
                    msg_record(topic.size());
                    msg_record(topic);
                }
            }
        }

        printf("receiver process is terminated...\n");
    }

public:
    void Init(std::unordered_map<std::string, std::string>& args) {
        std::vector<std::string> ports;

        int idx = 0;
        while (idx < args["port"].length()) {
            if (args["port"][idx] == '|') {
                break;
            }
            idx++;
        }
        if (idx == args["port"].length()) {
            ports.push_back(args["port"]);
        } else {
            ports = split(args["port"], '|');
        }
        for (auto& port : ports) {
            std::string uri = "tcp://" + args["src"] + ":" + port;
            std::string topic = args["topic"];
            std::string mode = args["mode"];

            uint32_t ip = ipStringToUint32(args["src"]);

            _sockets.emplace_back(_context, mode == "sub" ? ZMQ_SUB : ZMQ_PULL);
            zmq::socket_t& sock = _sockets.back();
            _socket_src_map[sock] = {ip, std::stoi(port)};
            init_recv_socket(sock, uri, topic, mode);
        }
    };

    void Run() {
        receive_thread = std::thread(&Recorder::receive_process, this);
    }

    void Stop() {
        this->_active_flag = 0;
        receive_thread.join();

        // 关闭 sockets
        for (auto& x : _sockets) {
            x.close();
        }

        // 销毁 ZeroMQ 上下文
        _context.close();

        // 关闭文件
        if (outFile.is_open()) {
            // 写入剩下所有缓存
            writeFile(nullptr, 0, true);
            outFile.close();
        }
    }
};

#endif
