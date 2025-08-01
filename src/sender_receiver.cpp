#include "sender_receiver.hpp"
#include "ffmpeg_encoder.h"
#include "slicer.hpp"
#include "erasure_coder.hpp"
#include "udp_sender.hpp"
#include "packet_parser.hpp"
#include "smart_collector.hpp"
#include "decode_and_display.hpp"

#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <fcntl.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;
using namespace cv;
typedef chrono::steady_clock Clock;

enum StatField { CAPTURE, ENCODE, SEND, DISPLAY, FIELD_COUNT };

void run_sender(const string& target_ip, const vector<int>& target_ports) {
    if (target_ports.empty()) {
        cerr << "Usage: sender <target_ip> <port1> [port2 ...]" << endl;
        exit(1);
    }

    if (!init_udp_sockets(target_ports.size())) {
        cerr << "[ERROR] UDP sockets could not be initialized." << endl;
        exit(1);
    }

    int width = 640, height = 480, fps = 30, bitrate = 600000;
    VideoCapture cap(0, cv::CAP_V4L2);
    cap.set(CAP_PROP_FRAME_WIDTH, width);
    cap.set(CAP_PROP_FRAME_HEIGHT, height);
    cap.set(CAP_PROP_FPS, fps);
    if (!cap.isOpened()) {
        cerr << "Camera could not be started." << endl;
        exit(1);
    }

    FFmpegEncoder encoder(width, height, fps, bitrate);
    uint16_t frame_id = 0;
    Mat frame;
    ErasureCoder fec(8, 4); // k = 8, r = 4

    chrono::milliseconds frame_duration(1000 / fps);
    double stats[FIELD_COUNT] = {0};
    int frame_count = 0;
    auto stats_start = Clock::now();

    while (true) {
        auto t0 = Clock::now();

        auto tc0 = Clock::now();
        cap.read(frame);
        auto tc1 = Clock::now();
        stats[CAPTURE] += chrono::duration<double, milli>(tc1 - tc0).count();

        if (!frame.empty()) {
            auto te0 = Clock::now();
            vector<uint8_t> encoded;
            encoder.encodeFrame(frame, encoded);
            auto te1 = Clock::now();
            stats[ENCODE] + chrono::duration<double, milli>(te1 - te0).count();

            auto ts0 = Clock::now();
            auto chunks = slice_frame(encoded, frame_id, 1000);

            vector<vector<uint8_t>> k_blocks;
            for (int i = 0; i < 8 && i < chunks.size(); ++i)
                k_blocks.push_back(chunks[i].payload);

            vector<vector<uint8_t>> all_blocks;
            fec.encode(k_blocks, all_blocks);

            for (int i = 0; i < all_blocks.size(); ++i) {
                ChunkPacket pkt;
                pkt.frame_id = frame_id;
                pkt.chunk_id = i;
                pkt.total_chunks = all_blocks.size();
                pkt.payload = all_blocks[i];

                for (int p = 0; p < target_ports.size(); ++p) {
                    send_udp(target_ip, target_ports[p], pkt);
                }
            }

            auto ts1 = Clock::now();
            stats[SEND] += chrono::duration<double, milli>(ts1 - ts0).count();
            frame_id++;
        }

        auto td0 = Clock::now();
        imshow("NovaEngine - Sender (You)", frame);
        if (waitKey(1) >= 0) break;
        auto td1 = Clock::now();
        stats[DISPLAY] += chrono::duration<double, milli>(td1 - td0).count();

        frame_count++;
        auto now = Clock::now();
        if (chrono::duration_cast<chrono::seconds>(now - stats_start).count() >= 1) {
            cout << "Avg times (ms): Capture=" << stats[CAPTURE]/frame_count
                 << ", Encode=" << stats[ENCODE]/frame_count
                 << ", Send=" << stats[SEND]/frame_count
                 << ", Display=" << stats[DISPLAY]/frame_count << endl;
            memset(stats, 0, sizeof(stats));
            frame_count = 0;
            stats_start = now;
        }

        auto t1 = Clock::now();
        auto loop_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0);
        if (loop_ms < frame_duration) {
            this_thread::sleep_for(frame_duration - loop_ms);
        }
    }

    close_udp_sockets();
    destroyAllWindows();
}

void run_receiver(const vector<int>& ports) {
    constexpr int MAX_BUFFER = 1500;
    const int disp_width = 640;
    const int disp_height = 480;

    vector<int> sockets;
    for (int port : ports) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (sockaddr*)&addr, sizeof(addr));
        fcntl(sock, F_SETFL, O_NONBLOCK);
        sockets.push_back(sock);
        cout << "Listening UDP " << port << endl;
    }

    H264Decoder decoder;
    Mat reconstructed_frame;
    bool has_received = false;

    SmartFrameCollector collector([&](const vector<uint8_t>& data) {
        if (decoder.decode(data, reconstructed_frame)) {
            has_received = true;
        }
    }, 8, 4); // k = 8, r = 4

    cout << "Receiver started..." << endl;

    while (true) {
        for (int sock : sockets) {
            uint8_t buf[MAX_BUFFER];
            ssize_t len = recv(sock, buf, sizeof(buf), 0);
            if (len >= 4) {
                auto pkt = parse_packet(buf, len);
                collector.handle(pkt);
            }
        }

        collector.flush_expired_frames();

        Mat display_frame;
        if (has_received && !reconstructed_frame.empty()) {
            Mat resized;
            resize(reconstructed_frame, resized, Size(disp_width, disp_height));
            display_frame = resized;
        } else {
            display_frame = Mat::zeros(disp_height, disp_width, CV_8UC3);
            putText(display_frame,
                    "Waiting for Client to Connect..",
                    Point(20, disp_height/2),
                    FONT_HERSHEY_SIMPLEX,
                    1.0,
                    Scalar(255,255,255),
                    2);
        }

        imshow("NovaEngine - Receiver", display_frame);
        if (waitKey(1) == 27) break;
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    for (int sock : sockets) close(sock);
    destroyAllWindows();
}
=
