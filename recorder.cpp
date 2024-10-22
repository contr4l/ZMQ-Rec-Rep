#include "zmq.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "parse_cfg.h"
#include "recorder.hpp"

std::atomic<bool> shutdown(false);

void signal_handler(int sig) {
    shutdown.store(true);
}

void usage() {
    printf("usage: zmq_recorder [options]\n");
    printf("options:\n");
    printf("  --mode        mode for recorder, [sub: default|rcv]\n");
    printf("  --src         src ip for sender, default 127.0.0.1\n");
    printf("  --port        src port for sender, default 9090\n");
    printf("  --topic       set topic filter string for sub mode, default empty string\n");
}

void print_cfg(std::unordered_map<std::string, std::string>& args) {
    printf("zmq recorder config:\n");
    printf("  mode:  %s\n", args["mode"].c_str());
    printf("  src:   %s\n", args["src"].c_str());
    printf("  port:  %s\n", args["port"].c_str());
    printf("  topic: %s\n", args["topic"].c_str());
}

void reg_default_kv(std::unordered_map<std::string, std::string>& args, std::string k, std::string v) {
    if (args.find(k) == args.end()) {
        args[k] = v;
    }
}

// Receiver receiver;
int main(int argc, char** argv) {
    std::shared_ptr<Recorder> recorder = std::make_shared<Recorder>();

    signal(SIGINT, signal_handler);
    std::unordered_map<std::string, std::string> args;

    try {
        args = parse_cfg(argc, argv);
        reg_default_kv(args, "mode", "sub");
        reg_default_kv(args, "src", "127.0.0.1");
        reg_default_kv(args, "port", "9090");
        reg_default_kv(args, "topic", "");
    }
    catch (std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        usage();
        exit(0);
    }

    print_cfg(args);

    // Perception result from zmq, to binary
    recorder->Init(args);
    recorder->Run();

    while (!shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    recorder->Stop();

    printf("Exiting gracefully...\n");
    return 0;
}