#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>
#include <vector>

class H264Decoder {
public:
    H264Decoder();
    ~H264Decoder();

    // Yeni frame decode edildiğinde true döner
    bool decode(const std::vector<uint8_t>& encoded_data, cv::Mat& output_frame);

private:
    AVCodec* codec = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* sws_ctx = nullptr;

    int width = 1280;
    int height = 720;

    void init();
    void cleanup();
};
