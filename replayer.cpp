#include "parse_cfg.h"
#include "zmq.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

#include "replayer.hpp"

std::atomic<bool> shutdown(false);

void signal_handler(int sig) {
    shutdown.store(true);
}

void usage() {
    printf("usage: zmq_replayer [options]\n");
    printf("options:\n");
    printf("  --file        src file for replay, default zmq.bin\n");
    printf("  --mode        mode for replayer, [pub: default|req]\n");
}

void print_cfg(std::unordered_map<std::string, std::string>& args) {
    printf("zmq zmq_replayer config:\n");
    printf("  file:  %s\n", args["file"].c_str());
    printf("  mode:  %s\n", args["mode"].c_str());
}

void reg_default_kv(std::unordered_map<std::string, std::string>& args, std::string k, std::string v) {
    if (args.find(k) == args.end()) {
        args[k] = v;
    }
}

// Replayer Replayer;
int main(int argc, char** argv) {
    std::shared_ptr<Replayer> replayer_ = std::make_shared<Replayer>();

    signal(SIGINT, signal_handler);
    std::unordered_map<std::string, std::string> args;

    try {
        args = parse_cfg(argc, argv);
        reg_default_kv(args, "mode", "pub");
        reg_default_kv(args, "file", "zmq.bin");
    }
    catch (std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        usage();
        exit(0);
    }

    print_cfg(args);

    // Perception result from zmq, to binary
    replayer_->Init(args.at("file"), args.at("mode"));
    replayer_->Run();

    while (!shutdown.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    replayer_->Stop();

    printf("Exiting gracefully...\n");
    return 0;
}