#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <my_ip> <my_port> <remote_ip> <remote_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.5 45000 192.168.1.10 45001" << std::endl;
        return 1;
    }

    std::string my_ip = argv[1];
    int my_port = std::stoi(argv[2]);
    std::string remote_ip = argv[3];
    int remote_port = std::stoi(argv[4]);

    std::cout << "Starting NovaEngine in bidirectional mode..." << std::endl;
    std::cout << "My IP: " << my_ip << ":" << my_port << std::endl;
    std::cout << "Remote IP: " << remote_ip << ":" << remote_port << std::endl;

    // Gönderici: kendi portumuzdan karşı tarafın portuna gönder
    std::vector<int> sender_ports = {my_port};
    std::vector<int> receiver_ports = {my_port};
    
    // Karşı tarafın adresini ve portunu gönderici için ayarla
    std::thread sender(run_sender, remote_ip, std::vector<int>{remote_port});
    std::thread receiver(run_receiver, receiver_ports);

    sender.join();
    receiver.join();
    return 0;
}
