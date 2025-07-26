#include "udp_sender.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

static std::vector<int> udp_sockets;

bool init_udp_sockets(size_t count) {
    udp_sockets.clear();

    for (size_t i = 0; i < count; ++i) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            return false;
        }
        udp_sockets.push_back(sock);
    }

    std::cout << "[udp_sender] ✅ " << count << " UDP socket hazırlandı.\n";
    return true;
}

void close_udp_sockets() {
    for (int sock : udp_sockets)
        close(sock);

    udp_sockets.clear();
    std::cout << "[udp_sender] Tüm UDP soketler kapatıldı.\n";
}

void send_chunks_to_ports(
    const std::vector<Chunk>& chunks,
    const std::string& target_ip,
    const std::vector<int>& target_ports
) {
    if (udp_sockets.empty() || target_ports.empty()) return;

    for (const Chunk& chunk : chunks) {
        for (size_t i = 0; i < target_ports.size(); ++i) {
            int port = target_ports[i];
            int sock = udp_sockets[i % udp_sockets.size()];  // gerekirse reuse

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);

            ssize_t sent = sendto(sock, chunk.data.data(), chunk.data.size(), 0,
                                  reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

            if (sent < 0)
                perror("[udp_sender]  Gönderim hatası");
            else
                std::cout << "[udp_sender] Chunk gönderildi → port " << port
                          << " | boyut: " << chunk.data.size() << " byte\n";
        }
    }
}
