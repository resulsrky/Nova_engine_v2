#ifndef FFMPEG_ENCODER_HPP
#define FFMPEG_ENCODER_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>
#include <vector>

class FFmpegEncoder {
public:
    FFmpegEncoder(int width, int height, int fps, int bitrate);
    ~FFmpegEncoder();

    bool encodeFrame(const cv::Mat& bgrFrame, std::vector<uint8_t>& outEncodedData);

private:
    int m_width, m_height, m_fps, m_bitrate;
    AVCodec* codec = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    SwsContext* swsCtx = nullptr;
    int frameCounter = 0;

    void initEncoder();
    void cleanup();
};

#endif // FFMPEG_ENCODER_HPP
