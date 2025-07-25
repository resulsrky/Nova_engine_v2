// sender_receiver.cpp

#include "sender_receiver.hpp"
#include "ffmpeg_encoder.h"
#include "slicer.hpp"
#include "udp_sender.hpp"
#include "packet_parser.hpp"
#include "smart_collector.hpp"
#include "decode_and_display.hpp"

#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace std;
using namespace cv;

void run_sender(int argc, char* argv[]) {
    string target_ip = argv[1];
    vector<int> target_ports;
    for (int i = 2; i < argc; ++i)
        target_ports.push_back(stoi(argv[i]));

    if (!init_udp_sockets(target_ports.size())) {
        cerr << "[ERROR] Socket re-Binding.\n";
        exit(1);
    }

    int width = 1280, height = 720, fps = 30, bitrate = 400000;
    VideoCapture cap(0, cv::CAP_V4L2);
    cap.set(CAP_PROP_FRAME_WIDTH, width);
    cap.set(CAP_PROP_FRAME_HEIGHT, height);
    cap.set(CAP_PROP_FPS, fps);
    if (!cap.isOpened()) {
        cerr << "Camera could not started.\n";
        exit(-1);
    }

    FFmpegEncoder encoder(width, height, fps, bitrate);
    uint16_t frame_id = 0;
    Mat frame;

    cout << "\U0001F4E4 Stream is starting (Sender)...\n";
    while (true) {
        cap.read(frame);
        if (frame.empty()) continue;

        vector<uint8_t> encoded;
        if (encoder.encodeFrame(frame, encoded)) {
            auto chunks = slice_frame(encoded.data(), encoded.size(), frame_id++);
            send_chunks_to_ports(chunks, target_ip, target_ports);
        }

        if (waitKey(1) >= 0) break;
    }

    close_udp_sockets();
}

void run_receiver() {
    constexpr int MAX_BUFFER = 1500;
    constexpr int PORTS[] = {45094, 44824, 33061, 55008};

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
        std::cout << "\U0001F50A Listening UDP " << port << "\n";
    }

    H264Decoder decoder;
    SmartFrameCollector collector([&](const std::vector<uint8_t>& data) {
        decoder.decode(data);  // gelen frame’i göster
    });

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
}
