#pragma once

#include "packet_parser.hpp"
#include "erasure_coder.hpp"
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <cstddef>

class SmartFrameCollector {
public:
    using FrameReadyCallback = std::function<void(const std::vector<uint8_t>&)>;

    SmartFrameCollector(FrameReadyCallback callback, int k, int r);
    ~SmartFrameCollector();
    void handle(ChunkPacket pkt);
    void flush_expired_frames();

private:
    struct PartialFrame {
        std::vector<std::vector<uint8_t>> chunks;
        std::vector<bool> received_flags;
        uint8_t total_chunks = 0;
        size_t received_chunks = 0;
        std::chrono::steady_clock::time_point last_update;
    };

    std::unordered_map<uint16_t, PartialFrame> frame_buffer;
    FrameReadyCallback callback;

    std::thread flush_thread;
    bool stop_flag = false;
    int timeout_ms_ = 15;

    ErasureCoder fec;
    int k_; // data chunks
    int r_; // parity chunks
};

