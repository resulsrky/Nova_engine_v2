#include "ffmpeg_encoder.h"
#include <iostream>

FFmpegEncoder::FFmpegEncoder(int width, int height, int fps, int bitrate)
    : m_width(width), m_height(height), m_fps(fps), m_bitrate(bitrate)
{
    avcodec_register_all();  // Optional in new FFmpeg versions
    initEncoder();
}

void FFmpegEncoder::initEncoder()
{
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        throw std::runtime_error("H264 encoder not found");
    }

    codecContext = avcodec_alloc_context3(codec);
    codecContext->width = m_width;
    codecContext->height = m_height;
    codecContext->time_base = AVRational{1, m_fps};
    codecContext->framerate = AVRational{m_fps, 1};
    codecContext->bit_rate = m_bitrate;
    codecContext->gop_size = 10;
    codecContext->max_b_frames = 1;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        throw std::runtime_error("Could not open codec");
    }

    frame = av_frame_alloc();
    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;
    av_frame_get_buffer(frame, 32);

    pkt = av_packet_alloc();

    swsCtx = sws_getContext(
        m_width, m_height, AV_PIX_FMT_BGR24,
        m_width, m_height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
}

bool FFmpegEncoder::encodeFrame(const cv::Mat& bgrFrame, std::vector<uint8_t>& outEncodedData)
{
    if (!codecContext || !frame || !pkt || bgrFrame.empty())
        return false;

    const uint8_t* inData[1] = { bgrFrame.data };
    int inLinesize[1] = { static_cast<int>(bgrFrame.step) };

    sws_scale(swsCtx, inData, inLinesize, 0, m_height, frame->data, frame->linesize);

    frame->pts = frameCounter++;

    if (avcodec_send_frame(codecContext, frame) < 0)
        return false;

    int ret = avcodec_receive_packet(codecContext, pkt);
    if (ret == 0) {
        outEncodedData.assign(pkt->data, pkt->data + pkt->size);
        av_packet_unref(pkt);
        return true;
    }

    return false;
}

void FFmpegEncoder::cleanup()
{
    if (codecContext) avcodec_free_context(&codecContext);
    if (frame) av_frame_free(&frame);
    if (pkt) av_packet_free(&pkt);
    if (swsCtx) sws_freeContext(swsCtx);
}

FFmpegEncoder::~FFmpegEncoder()
{
    cleanup();
}
