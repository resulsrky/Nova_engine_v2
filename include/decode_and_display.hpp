#pragma once
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class H264Decoder {
public:
    H264Decoder() {
        avcodec_register_all();
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "H264 codec bulunamadı.\n";
            std::exit(1);
        }

        ctx = avcodec_alloc_context3(codec);
        if (avcodec_open2(ctx, codec, nullptr) < 0) {
            std::cerr << "Codec açılamadı.\n";
            std::exit(1);
        }

        parser = av_parser_init(AV_CODEC_ID_H264);
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
    }

    ~H264Decoder() {
        avcodec_free_context(&ctx);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        av_parser_close(parser);
    }

    void decode(const std::vector<uint8_t>& input) {
        const uint8_t* data = input.data();
        size_t size = input.size();

        while (size > 0) {
            int parsed_len = av_parser_parse2(parser, ctx, &pkt->data, &pkt->size,
                                              data, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (parsed_len < 0) break;

            data += parsed_len;
            size -= parsed_len;

            if (pkt->size > 0)
                decode_packet(pkt);
        }
    }

private:
    AVCodec* codec = nullptr;
    AVCodecContext* ctx = nullptr;
    AVCodecParserContext* parser = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;

    void decode_packet(AVPacket* pkt) {
        int ret = avcodec_send_packet(ctx, pkt);
        if (ret < 0) return;

        while (ret >= 0) {
            ret = avcodec_receive_frame(ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return;

            show_frame(frame);
        }
    }

    void show_frame(AVFrame* f) {
        int w = f->width;
        int h = f->height;
        AVPixelFormat fmt = static_cast<AVPixelFormat>(f->format);

        SwsContext* sws = sws_getContext(w, h, fmt,
                                         w, h, AV_PIX_FMT_BGR24,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr);

        cv::Mat img(h, w, CV_8UC3);
        uint8_t* dest[4] = { img.data, nullptr, nullptr, nullptr };
        int linesize[4] = { img.step[0], 0, 0, 0 };

        sws_scale(sws, f->data, f->linesize, 0, h, dest, linesize);
        sws_freeContext(sws);

        cv::imshow("🎥 NovaEngine Receiver", img);
        cv::waitKey(1);
    }
};
