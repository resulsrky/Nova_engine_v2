#include "ffmpeg_encoder.h"
#include <stdexcept>
#include <iostream>
#include <thread>
#include <opencv2/imgproc.hpp>

FFmpegEncoder::FFmpegEncoder(int width, int height, int fps, int bitrate)
    : m_width(width), m_height(height), m_fps(fps), m_bitrate(bitrate)
{
    initEncoder();
}

void FFmpegEncoder::initEncoder() {
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
        throw std::runtime_error("H264 codec not found");

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
        throw std::runtime_error("Failed to allocate codec context");

    codecContext->width = m_width;
    codecContext->height = m_height;
    codecContext->time_base = {1, m_fps};
    codecContext->framerate = {m_fps, 1};
    codecContext->bit_rate = m_bitrate;
    codecContext->gop_size = 10;
    codecContext->max_b_frames = 1;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Enhanced threading configuration
    codecContext->thread_count = std::thread::hardware_concurrency();
    codecContext->thread_type = FF_THREAD_FRAME; // Frame-level threading
    
    // Set optimal encoding parameters for low latency
    av_opt_set(codecContext->priv_data, "threads", "auto", 0);
    av_opt_set(codecContext->priv_data, "preset", "superfast", 0);
    av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);
    av_opt_set(codecContext->priv_data, "profile", "baseline", 0);
    
    // Low latency settings
    av_opt_set(codecContext->priv_data, "bf", "0", 0); // No B-frames for lower latency
    av_opt_set(codecContext->priv_data, "refs", "1", 0); // Single reference frame
    av_opt_set(codecContext->priv_data, "g", "10", 0); // GOP size
    av_opt_set(codecContext->priv_data, "keyint_min", "10", 0);
    av_opt_set(codecContext->priv_data, "sc_threshold", "0", 0); // Disable scene cut detection
    av_opt_set(codecContext->priv_data, "flags", "+cgop", 0); // Closed GOP

    if (avcodec_open2(codecContext, codec, nullptr) < 0)
        throw std::runtime_error("Failed to open codec");

    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    if (!frame || !pkt)
        throw std::runtime_error("Failed to allocate frame or packet");

    frame->format = codecContext->pix_fmt;
    frame->width = codecContext->width;
    frame->height = codecContext->height;

    if (av_frame_get_buffer(frame, 32) < 0)
        throw std::runtime_error("Could not allocate frame buffer");

    swsCtx = sws_getContext(
        m_width, m_height, AV_PIX_FMT_BGR24,
        m_width, m_height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    if (!swsCtx)
        throw std::runtime_error("Failed to initialize swscale context");
}

// Calculate scene complexity for dynamic compression
double FFmpegEncoder::calculateSceneComplexity(const cv::Mat& bgrFrame) {
    if (bgrFrame.empty()) return 0.0;
    
    cv::Mat gray, edges;
    cv::cvtColor(bgrFrame, gray, cv::COLOR_BGR2GRAY);
    
    // Calculate edge density as complexity measure
    cv::Canny(gray, edges, 50, 150);
    double edge_ratio = cv::countNonZero(edges) / static_cast<double>(edges.total());
    
    // Calculate variance as another complexity measure
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    double variance = stddev[0] * stddev[0];
    
    // Normalize and combine metrics
    double complexity = (edge_ratio * 1000.0 + variance / 100.0) / 2.0;
    return std::min(complexity, 1.0); // Normalize to [0,1]
}

bool FFmpegEncoder::encodeFrame(const cv::Mat& bgrFrame, std::vector<uint8_t>& outEncodedData) {
    if (!codecContext || !frame || !pkt || bgrFrame.empty())
        return false;

    // Calculate scene complexity and adjust encoding parameters
    double complexity = calculateSceneComplexity(bgrFrame);
    
    // Dynamic compression based on scene complexity
    if (complexity > 0.7) {
        // High complexity: use higher quality settings
        av_opt_set(codecContext->priv_data, "crf", "18", 0);
        av_opt_set(codecContext->priv_data, "preset", "fast", 0);
    } else if (complexity > 0.3) {
        // Medium complexity: balanced settings
        av_opt_set(codecContext->priv_data, "crf", "23", 0);
        av_opt_set(codecContext->priv_data, "preset", "superfast", 0);
    } else {
        // Low complexity: faster encoding
        av_opt_set(codecContext->priv_data, "crf", "28", 0);
        av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0);
    }

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

void FFmpegEncoder::setBitrate(int bitrate) {
    if (!codecContext) return;
    if (bitrate == m_bitrate) return;

    m_bitrate = bitrate;
    
    // Update bitrate without reinitializing the codec
    codecContext->bit_rate = m_bitrate;
    
    // Also update related parameters for better quality control
    int target_bpp = (bitrate * 1000) / (m_width * m_height * m_fps); // bits per pixel
    if (target_bpp > 0) {
        // Adjust CRF based on target bitrate
        int crf = std::max(18, std::min(28, 30 - (target_bpp / 2)));
        av_opt_set(codecContext->priv_data, "crf", std::to_string(crf).c_str(), 0);
    }
    
    std::cout << "[ENCODER] Bitrate updated to " << (bitrate/1000) << " kbps" << std::endl;
}

void FFmpegEncoder::cleanup() {
    if (codecContext) avcodec_free_context(&codecContext);
    if (frame) av_frame_free(&frame);
    if (pkt) av_packet_free(&pkt);
    if (swsCtx) sws_freeContext(swsCtx);
}

FFmpegEncoder::~FFmpegEncoder() {
    cleanup();
}
