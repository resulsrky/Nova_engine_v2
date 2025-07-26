#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <opencv2/core.hpp>
#include <vector>

class FFmpegEncoder {
public:
    FFmpegEncoder(int width, int height, int fps, int bitrate);
    ~FFmpegEncoder();

    // BGR Mat -> H264 encode edilmiş veri
    bool encodeFrame(const cv::Mat& bgrFrame, std::vector<uint8_t>& outEncodedData);

private:
    int m_width, m_height, m_fps, m_bitrate;
    int frameCounter = 0;

    AVCodec* codec = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    SwsContext* swsCtx = nullptr;

    void initEncoder();
    void cleanup();
};
