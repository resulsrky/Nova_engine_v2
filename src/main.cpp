#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <my_ip> <my_port> <remote_ip> <remote_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.5 45000 192.168.1.10 45001" << std::endl;
        std::cerr << "Each side uses different ports for bidirectional communication" << std::endl;
        return 1;
    }

    std::string my_ip = argv[1];
    int my_port = std::stoi(argv[2]);
    std::string remote_ip = argv[3];
    int remote_port = std::stoi(argv[4]);

    std::cout << "Starting NovaEngine in bidirectional mode..." << std::endl;
    std::cout << "My IP: " << my_ip << ":" << my_port << std::endl;
    std::cout << "Remote IP: " << remote_ip << ":" << remote_port << std::endl;

    // Her taraf kendi portunu dinliyor, karşı tarafın portuna gönderiyor
    std::vector<int> my_ports = {my_port};
    std::vector<int> remote_ports = {remote_port};
    
    // Gönderici ve alıcı farklı portlar kullanıyor
    std::thread sender(run_sender, remote_ip, remote_ports);
    std::thread receiver(run_receiver, my_ports);

    sender.join();
    receiver.join();
    return 0;
}
