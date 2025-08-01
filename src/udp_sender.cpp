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

void send_udp(const std::string& target_ip, int port, const ChunkPacket& packet) {
    if (udp_sockets.empty()) return;

    int sock = udp_sockets[port % udp_sockets.size()];  // socket reuse

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);

    // serialize
    std::vector<uint8_t> buffer = serialize_packet(packet);

    ssize_t sent = sendto(sock, buffer.data(), buffer.size(), 0,
                          reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (sent < 0)
        perror("[udp_sender] Gönderim hatası");
    else
        std::cout << "[udp_sender] Chunk gönderildi → port " << port
                  << " | boyut: " << buffer.size() << " byte\n";
}
