#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./novaengine <public_ip> <port1> <port2> ...\n";
        return 1;
    }

    std::string target_ip = argv[1];
    std::vector<int> target_ports;
    for (int i = 2; i < argc; ++i) {
        target_ports.push_back(std::stoi(argv[i]));
    }

    // Sender thread
    std::thread sender_thread([&]() {
        std::vector<std::string> args = {"sender", target_ip};
        for (int port : target_ports)
            args.push_back(std::to_string(port));

        std::vector<char*> argv_c;
        for (auto& arg : args)
            argv_c.push_back(arg.data());

        run_sender(argv_c.size(), argv_c.data());
    });

    // Receiver thread
    std::thread receiver_thread(run_receiver);

    sender_thread.join();
    receiver_thread.join();

    return 0;
}
