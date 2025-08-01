#include "udp_sender.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <errno.h>

static std::vector<int> udp_sockets;

bool init_udp_sockets(const std::vector<int>& local_ports) {
    udp_sockets.clear();

    for (int port : local_ports) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            perror("bind failed");
            close(sock);
            return false;
        }
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        udp_sockets.push_back(sock);
    }

    std::cout << "[udp_sender] ✅ " << local_ports.size() << " UDP socket hazırlandı.\n";
    return true;
}

void close_udp_sockets() {
    for (int sock : udp_sockets)
        close(sock);

    udp_sockets.clear();
    std::cout << "[udp_sender] Tüm UDP soketler kapatıldı.\n";
}

ssize_t send_udp(const std::string& target_ip, int port, const ChunkPacket& packet) {
    if (udp_sockets.empty()) return -1;

    int sock = udp_sockets[port % udp_sockets.size()];  // socket reuse

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);

    // serialize
    std::vector<uint8_t> buffer = serialize_packet(packet);

    ssize_t sent;
    do {
        sent = sendto(sock, buffer.data(), buffer.size(), 0,
                      reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    } while (sent < 0 && errno == EAGAIN);

    if (sent < 0) {
        perror("[udp_sender] Gönderim hatası");
    }
    return sent;
}
