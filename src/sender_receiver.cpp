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

void run_sender(const string& target_ip, const vector<int>& target_ports) {
    if (target_ports.empty()) {
        cerr << "Usage: sender <target_ip> <port1> <port2> ..." << endl;
        exit(1);
    }

    if (!init_udp_sockets(target_ports.size())) {
        cerr << "[ERROR] UDP sockets could not be initialized." << endl;
        exit(1);
    }

    int width = 1280, height = 720;
    int fps = 30;
    int bitrate = 400000;
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

    cout << "Stream started (Sender)..." << endl;

    // FPS control
    chrono::milliseconds frame_duration(1000 / fps);
    int frame_count = 0;
    auto start_time = chrono::steady_clock::now();

    while (true) {
        auto loop_start = chrono::steady_clock::now();

        cap.read(frame);
        if (frame.empty()) continue;

        // Measure encode time
        auto t0 = chrono::steady_clock::now();
        vector<uint8_t> encoded;
        bool ok = encoder.encodeFrame(frame, encoded);
        auto t1 = chrono::steady_clock::now();
        auto encode_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

        if (ok) {
            auto chunks = slice_frame(encoded.data(), encoded.size(), frame_id++);
            send_chunks_to_ports(chunks, target_ip, target_ports);
        }

        imshow("NovaEngine - Sender (You)", frame);

        // FPS measurement and log every second
        frame_count++;
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - start_time).count();
        if (elapsed >= 1) {
            double actual_fps = frame_count / double(elapsed);
            cout << "Actual FPS: " << actual_fps << " | Encode time: " << encode_ms << " ms" << endl;
            frame_count = 0;
            start_time = now;
        }

        // Throttle to target FPS
        auto loop_end = chrono::steady_clock::now();
        auto loop_time = chrono::duration_cast<chrono::milliseconds>(loop_end - loop_start);
        if (loop_time < frame_duration) {
            this_thread::sleep_for(frame_duration - loop_time);
        }

        if (waitKey(1) >= 0) break;
    }

    close_udp_sockets();
    destroyAllWindows();
}

void run_receiver(const vector<int>& ports) {
    constexpr int MAX_BUFFER = 1500;
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
    });

    cout << "Receiver started..." << endl;

    while (true) {
        for (int sock : sockets) {
            uint8_t buf[MAX_BUFFER];
            ssize_t len = recv(sock, buf, sizeof(buf), 0);
            if (len >= 4) {
                ChunkPacket pkt = parse_packet(buf, len);
                collector.handle(pkt);
            }
        }

        collector.flush_expired_frames();

        Mat display_frame;
        if (has_received && !reconstructed_frame.empty()) {
            display_frame = reconstructed_frame.clone();
        } else {
            display_frame = Mat::zeros(720, 1280, CV_8UC3);
            putText(display_frame, "Waiting for Client to Connect..", Point(200, 360), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(255,255,255), 3);
        }

        imshow("NovaEngine - Receiver", display_frame);

        if (waitKey(1) == 27) break;
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    for (int sock : sockets) close(sock);
    destroyAllWindows();
}
