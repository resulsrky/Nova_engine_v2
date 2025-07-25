#include "packet_parser.hpp"
#include "smart_collector.hpp"
#include "decode_and_display.hpp"

#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <iostream>
#include <thread>

constexpr int MAX_BUFFER = 1500;

void run_receiver(const std::vector<int>& port_list) {
    std::vector<int> sockets;
    for (int port : port_list) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Socket oluşturulamadı: " << port << "\n";
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Bind başarısız: " << port << "\n";
            close(sock);
            continue;
        }

        fcntl(sock, F_SETFL, O_NONBLOCK);
        sockets.push_back(sock);
        std::cout << "Listening: UDP " << port << "\n";
    }

    H264Decoder decoder;
    SmartFrameCollector collector([&](const std::vector<uint8_t>& data) {
        decoder.decode(data);  // reconstructed frame'leri göster
    });

    std::cout << "Receiver başlatıldı...\n";

    while (true) {
        for (int sock : sockets) {
            uint8_t buf[MAX_BUFFER];
            ssize_t len = recv(sock, buf, sizeof(buf), 0);
            if (len >= 4) {
                ChunkPacket pkt = parse_packet(buf, len);
                collector.handle(pkt);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (int sock : sockets)
        close(sock);
}
