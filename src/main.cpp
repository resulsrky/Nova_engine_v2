#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <string>


#include "ffmpeg_encoder.h"
#include "slicer.hpp"
#include "udp_sender.hpp"

using namespace cv;
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usafe: ./novaengine <TargetIP> <Port1> [Port2] [Port3] ...\n";
        return 1;
    }

    string target_ip = argv[1];
    vector<int> target_ports;
    for (int i = 2; i < argc; ++i)
        target_ports.push_back(stoi(argv[i]));

    if (!init_udp_sockets(target_ports.size())) {
        cerr << "[ERROR] Socket re-Binding.\n";
        return 1;
    }

    int width = 1280, height = 720, fps = 30, bitrate = 400000;
    VideoCapture cap(0, cv::CAP_V4L2);
    cap.set(CAP_PROP_FRAME_WIDTH, width);
    cap.set(CAP_PROP_FRAME_HEIGHT, height);
    cap.set(CAP_PROP_FPS, fps);
    if (!cap.isOpened()) {
        cerr << "Camera could not started.\n";
        return -1;
    }

    FFmpegEncoder encoder(width, height, fps, bitrate);
    uint16_t frame_id = 0;
    Mat frame;

    cout << "Stream is starting...\n";
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
    return 0;
}
