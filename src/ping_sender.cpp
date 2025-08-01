#include "ping_sender.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

constexpr uint32_t PING_MAGIC = 0xFEEDFACE;

static uint64_t now_ns() {
    return chrono::duration_cast<chrono::nanoseconds>(
        chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void send_ping_chunks(const string& ip, const vector<int>& ports) {
    for (int port : ports) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("Socket create error");
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

        PingChunk ping{};
        ping.magic = PING_MAGIC;
        ping.timestamp_ns = now_ns();

        ssize_t sent = sendto(sock, &ping, sizeof(ping), 0,
                              reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (sent > 0) {
            cout << "[ping_sender] Ping gönderildi → " << ip << ":" << port << "\n";
        }

        close(sock);
    }
}

void listen_for_ping_response(int listen_port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket create failed");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return;
    }

    char buffer[1024];
    while (true) {
        sockaddr_in from{};
        socklen_t fromlen = sizeof(from);
        ssize_t n = recvfrom(sock, buffer, sizeof(buffer), 0,
                             reinterpret_cast<sockaddr*>(&from), &fromlen);

        if (n >= sizeof(PingChunk)) {
            PingChunk* reply = reinterpret_cast<PingChunk*>(buffer);
            if (reply->magic == PING_MAGIC) {
                uint64_t now = now_ns();
                uint64_t rtt_ns = now - reply->timestamp_ns;

                cout << "[ping_sender] RTT = " << rtt_ns / 1'000'000.0 << " ms\n";
            }
        }
    }
}

