#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <public_ip> <port1> [port2 ...]" << std::endl;
        return 1;
    }

    std::string public_ip = argv[1];
    std::vector<int> ports;
    for (int i = 2; i < argc; ++i) {
        ports.push_back(std::stoi(argv[i]));
    }

    std::thread sender(run_sender, public_ip, ports);
    std::thread receiver(run_receiver, ports);

    sender.join();
    receiver.join();
    return 0;
}
