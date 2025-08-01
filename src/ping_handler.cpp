#include "ping_handler.hpp"
#include "ping_sender.hpp"  // PingChunk struct için
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

using namespace std;

constexpr uint32_t PING_MAGIC = 0xFEEDFACE;

void start_ping_responder(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Responder socket error");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("Responder bind failed");
        close(sock);
        return;
    }

    cout << "[ping_handler] Ping cevaplayıcı aktif, port: " << port << "\n";

    char buffer[1024];
    while (true) {
        sockaddr_in client{};
        socklen_t clen = sizeof(client);
        ssize_t n = recvfrom(sock, buffer, sizeof(buffer), 0,
                             reinterpret_cast<sockaddr*>(&client), &clen);

        if (n >= sizeof(PingChunk)) {
            PingChunk* req = reinterpret_cast<PingChunk*>(buffer);
            if (req->magic == PING_MAGIC) {
                ssize_t sent = sendto(sock, buffer, sizeof(PingChunk), 0,
                                      reinterpret_cast<sockaddr*>(&client), clen);
                if (sent > 0)
                    cout << "[ping_handler] Ping yanıtlandı.\n";
            }
        }
    }
}

