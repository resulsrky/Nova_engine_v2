#include <iostream>
#include <vector>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "packet_parser.hpp"
#include "smart_collector.hpp"
#include "decode_and_display.hpp"

constexpr int MAX_BUFFER = 1500;
constexpr int PORTS[] = {45094, 44824, 33061, 55008};


int main() {
    std::vector<int> sockets;
    for (int port : PORTS) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (sockaddr*)&addr, sizeof(addr));
        fcntl(sock, F_SETFL, O_NONBLOCK);
        sockets.push_back(sock);
        std::cout << "Listening: UDP " << port << "\n";
    }

    SmartFrameCollector collector([](const std::vector<uint8_t>& data) {
        static int frame_id = 0;
        std::string filename = "frame_" + std::to_string(frame_id++) + ".h264";
        FILE* f = fopen(filename.c_str(), "wb");
        fwrite(data.data(), 1, data.size(), f);
        fclose(f);
        std::cout << "Frame reconstructed (" << data.size() << " bytes)\n";
    });
    H264Decoder decoder;

    while (true) {
        for (int sock : sockets) {
            uint8_t buf[MAX_BUFFER];
            ssize_t len = recv(sock, buf, sizeof(buf), 0);
            if (len >= 4) {
                ChunkPacket pkt = parse_packet(buf, len);
                collector.handle(pkt);

                SmartFrameCollector collector([&](const std::vector<uint8_t>& data) {
                    decoder.decode(data);  // gelen frame’i göster
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
