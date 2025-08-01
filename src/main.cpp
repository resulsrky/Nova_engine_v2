#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <my_ip> <shared_port> <remote_ip>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.5 45000 192.168.1.10" << std::endl;
        std::cerr << "Both sides use the same port for bidirectional communication" << std::endl;
        return 1;
    }

    std::string my_ip = argv[1];
    int shared_port = std::stoi(argv[2]);
    std::string remote_ip = argv[3];

    std::cout << "Starting NovaEngine in bidirectional mode with shared port..." << std::endl;
    std::cout << "My IP: " << my_ip << ":" << shared_port << std::endl;
    std::cout << "Remote IP: " << remote_ip << ":" << shared_port << std::endl;

    // Her iki taraf da aynı portu kullanıyor
    std::vector<int> shared_ports = {shared_port};
    
    // Gönderici ve alıcı aynı portu kullanıyor
    std::thread sender(run_sender, remote_ip, shared_ports);
    std::thread receiver(run_receiver, shared_ports);

    sender.join();
    receiver.join();
    return 0;
}
