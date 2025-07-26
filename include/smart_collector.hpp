#pragma once

#include "packet_parser.hpp"
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>

class SmartFrameCollector {
public:
    using FrameReadyCallback = std::function<void(const std::vector<uint8_t>&)>;

    SmartFrameCollector(FrameReadyCallback callback);
    void handle(const ChunkPacket& pkt);

    // Belirli aralıklarla çağrılmalı (örneğin her frame loop'unda)
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
};
