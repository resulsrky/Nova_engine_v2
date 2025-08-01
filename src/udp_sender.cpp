#include "udp_sender.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <errno.h>
#include <chrono>
#include <thread>

static std::vector<int> udp_sockets;
static std::vector<std::string> target_ips;
static std::vector<int> target_ports;
static std::vector<sockaddr_in> target_addrs;

bool init_udp_sockets(const std::vector<int>& local_ports) {
    udp_sockets.clear();

    for (int port : local_ports) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            return false;
        }
        
        // Set socket options for low latency
        int optval = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        
        // Set send buffer size for better throughput
        int sendbuf = 65536;
        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
        
        // Set non-blocking mode
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        
        // Bind to specific port for receiver, but let system assign port for sender
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            perror("bind failed");
            close(sock);
            return false;
        }
        
        udp_sockets.push_back(sock);
    }

    std::cout << "[udp_sender] ✅ " << local_ports.size() << " UDP sockets prepared.\n";
    return true;
}

void close_udp_sockets() {
    for (int sock : udp_sockets)
        close(sock);

    udp_sockets.clear();
    target_addrs.clear();
    std::cout << "[udp_sender] All UDP sockets closed.\n";
}

// Pre-compute target addresses for zero-copy optimization
void set_target_addresses(const std::string& target_ip, const std::vector<int>& ports) {
    target_addrs.clear();
    target_ips.clear();
    target_ports.clear();
    
    for (int port : ports) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);
        target_addrs.push_back(addr);
        target_ips.push_back(target_ip);
        target_ports.push_back(port);
    }
}

ssize_t send_udp(const std::string& target_ip, int port, const ChunkPacket& packet) {
    if (udp_sockets.empty()) return -1;

    // Find the target address
    size_t addr_index = 0;
    bool found = false;
    for (size_t i = 0; i < target_ports.size(); ++i) {
        if (target_ports[i] == port && target_ips[i] == target_ip) {
            addr_index = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Add new target address
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, target_ip.c_str(), &addr.sin_addr);
        target_addrs.push_back(addr);
        target_ips.push_back(target_ip);
        target_ports.push_back(port);
        addr_index = target_addrs.size() - 1;
    }

    // Select socket using round-robin for load balancing
    static size_t socket_index = 0;
    int sock = udp_sockets[socket_index % udp_sockets.size()];
    socket_index++;

    // Serialize packet
    std::vector<uint8_t> buffer = serialize_packet(packet);

    // Non-blocking send with retry
    ssize_t sent = 0;
    int retries = 0;
    const int max_retries = 3;
    
    while (retries < max_retries) {
        sent = sendto(sock, buffer.data(), buffer.size(), MSG_DONTWAIT,
                      reinterpret_cast<sockaddr*>(&target_addrs[addr_index]), 
                      sizeof(sockaddr_in));
        
        if (sent >= 0) {
            break; // Success
        }
        
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Buffer full, try again after a short delay
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            retries++;
        } else {
            // Other error, don't retry
            perror("[udp_sender] Send error");
            break;
        }
    }

    if (sent < 0 && retries >= max_retries) {
        std::cerr << "[udp_sender] Failed to send after " << max_retries << " retries" << std::endl;
    }
    
    return sent;
}

// Enhanced send with multiple paths for redundancy
ssize_t send_udp_multipath(const std::string& target_ip, const std::vector<int>& ports, 
                          const ChunkPacket& packet) {
    if (udp_sockets.empty()) return -1;

    ssize_t total_sent = 0;
    std::vector<ssize_t> sent_per_path;
    
    // Send to all paths for redundancy
    for (int port : ports) {
        ssize_t sent = send_udp(target_ip, port, packet);
        sent_per_path.push_back(sent);
        if (sent > 0) total_sent += sent;
    }
    
    // Log if some paths failed
    int failed_paths = 0;
    for (ssize_t sent : sent_per_path) {
        if (sent < 0) failed_paths++;
    }
    
    if (failed_paths > 0 && failed_paths < static_cast<int>(ports.size())) {
        std::cout << "[udp_sender] Sent via " << (ports.size() - failed_paths) 
                 << "/" << ports.size() << " paths" << std::endl;
    }
    
    return total_sent;
}
