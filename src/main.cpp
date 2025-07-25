#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <chrono>
#include "ffmpeg_encoder.h"
#include "slicer.hpp"

using namespace cv;
using namespace std;
void process_encoded_frame(const uint8_t* encoded_data, size_t encoded_size) {
    static uint16_t frame_id = 0;

    auto chunks = slice_frame(encoded_data, encoded_size, frame_id++);
    for (const auto& chunk : chunks) {
        // Burada UDP ile göndereceğiz (2. adıma geçince)
        printf("Chunk size: %zu\n", chunk.data.size());
    }
}
int main()
{
    Mat frame;
    VideoCapture cap;
    int width = 1280;
    int height = 720;
    int fps = 30;
    int bitrate = 400000; // 400 kbps

    int deviceID = 0;
    int apiID = cv::CAP_ANY;
    cap.set(CAP_PROP_FRAME_WIDTH, width);
    cap.set(CAP_PROP_FRAME_HEIGHT, height);
    cap.open(deviceID, apiID);
    cap.set(CAP_PROP_FPS, fps);

    if (!cap.isOpened()) {
        cerr << "Unable to open camera device" << endl;
        return -1;
    }

    FFmpegEncoder encoder(width, height, fps, bitrate);

    int frame_count = 0;
    auto start = chrono::steady_clock::now();
    auto warmup_start = chrono::steady_clock::now();
    bool warmupDone = false;

    cout << "Warming up the camera for 3 seconds..." << endl;

    while (true) {
        cap.read(frame);
        if (frame.empty()) {
            cerr << "Empty frame" << endl;
            continue;
        }

        auto now = chrono::steady_clock::now();
        double warmupElapsed = chrono::duration<double>(now - warmup_start).count();

        if (!warmupDone) {
            imshow("Warming up...", frame);
            if (warmupElapsed >= 0.1) {
                destroyWindow("Warming up...");
                warmupDone = true;
                cout << "Warmup complete. Starting encoding..." << endl;
                start = chrono::steady_clock::now();
            }
            waitKey(1);
            continue;
        }

        std::vector<uint8_t> encoded;
        if (encoder.encodeFrame(frame, encoded)) {
            cout << "Encoded frame size: " << encoded.size() << " bytes" << endl;
            // TODO: Send via UDP
        }

        imshow("Live", frame);
        if (waitKey(1) >= 0)
            break;

        frame_count++;
        if (frame_count == 60) {
            auto end = chrono::steady_clock::now();
            double elapsed_sec = chrono::duration<double>(end - start).count();
            cout << "Measured FPS: " << frame_count / elapsed_sec << endl;
            frame_count = 0;
            start = chrono::steady_clock::now();
        }
    }

    return 0;
}
