#ifndef _ALGO_REPLAYER_H_
#define _ALGO_REPLAYER_H_

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <math.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

#include "parse_cfg.h"
#include "zmq.hpp"

constexpr int ms2s = 1000;
constexpr int ns2s = 1000000000;

enum class LandMarkType {
    Pillar = 12,
    Parklock = 13,
    Brump = 14,
};

#pragma pack(1)
typedef struct zmq_bag_header {
    uint32_t timestamp;
    uint32_t ip;
    uint16_t port;
    uint16_t topic_len;
    uint16_t data_len;
} zmq_bag_header_t;
#pragma pack()

class Replayer {
    // 接收zmq感知结果，发送至DDS
private:
    int _active_flag = 1;
    std::thread receive_thread;

private:
    std::ifstream inFile;
    std::string socket_mode;

    zmq::context_t _context;
    std::vector<zmq::socket_t> _sockets;
    std::unordered_map<uint64_t, zmq::socket_ref> _socket_src_map;

    uint64_t ip_port_hash(uint32_t ip, uint16_t port) {
        return ((uint64_t)ip << 16) | port;
    }

    void init_send_socket(zmq::socket_t& sock, std::string uri) {
        printf("init send socket %s\n", uri.c_str());
        sock.bind(uri);
    }

    void msg_dispatch(zmq::socket_ref socket, std::string topic, zmq::message_t& msg) {
        zmq::message_t topic_msg = zmq::message_t(topic.begin(), topic.end());
        socket.send(topic_msg, zmq::send_flags::sndmore);
        socket.send(msg, zmq::send_flags::none);
    }

    void readFile(zmq_bag_header_t& header, zmq::message_t& topic, zmq::message_t& data) {
        int topic_len = header.topic_len;
        int data_len = header.data_len;

        topic = zmq::message_t(topic_len + 1);
        data = zmq::message_t(data_len);

        inFile.read(static_cast<char*>(topic.data()), topic_len);
        ((char*)topic.data())[topic_len] = '\0';
        spdlog::info("read topic {}", topic.to_string());
        inFile.read(static_cast<char*>(data.data()), data_len);
    }

    void reg_socket(zmq_bag_header_t& header) {
        uint64_t hash = ip_port_hash(header.ip, header.port);
        if (_socket_src_map.find(hash) != _socket_src_map.end()) {
            return;
        }

        std::string uri = ipPortToTcpString(header.ip, header.port);
        _sockets.emplace_back(_context, (socket_mode == "pub") ? ZMQ_PUB : ZMQ_REQ);
        printf("init send socket %s\n", uri.c_str());
        zmq::socket_ref sock = _sockets.back();
        sock.bind(uri);

        _socket_src_map[hash] = sock;
    }

    void readFile(zmq_bag_header_t& header) {
        inFile.read((char*)&header, sizeof(header));
        reg_socket(header);
    }

    void replay_process() {
        if (!inFile.is_open()) {
            return;
        }
        zmq_bag_header_t header;
        zmq::message_t topic_msg;
        zmq::message_t data_msg;
        uint32_t interval_ms = 5;

        readFile(header);
        uint32_t curr_point = header.timestamp;
        while (_active_flag) {
            // 读取时间戳, 如果当前时间大于包的时间戳，则需要发送
            while (curr_point >= header.timestamp && !inFile.eof()) {
                readFile(header, topic_msg, data_msg);
                zmq::socket_ref socket = _socket_src_map[ip_port_hash(header.ip, header.port)];
                msg_dispatch(socket, topic_msg.to_string(), data_msg);

                // next bag
                readFile(header);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
            curr_point += interval_ms;
        }

        printf("Replayer process is terminated...\n");
    }

public:
    void Init(std::string filename, std::string mode) {
        inFile.open(filename, std::ios::binary | std::ios::in);
        if (!inFile.is_open()) {
            spdlog::warn("{} not existed!", filename);
            return;
        }

        socket_mode = mode;
        if (socket_mode != "pub" && socket_mode != "req") {
            spdlog::warn("{} not supported!", socket_mode);
            return;
        }
    };

    void Run() {
        receive_thread = std::thread(&Replayer::replay_process, this);
    }

    void Stop() {
        if (inFile.is_open()) {
            inFile.close();
        }
        this->_active_flag = 0;
        receive_thread.join();
    }
};

#endif
