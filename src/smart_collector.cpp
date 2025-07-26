#include "smart_collector.hpp"
#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
constexpr int JITTER_TIMEOUT_MS = 15;

SmartFrameCollector::SmartFrameCollector(FrameReadyCallback cb)
    : callback(std::move(cb)) {}

void SmartFrameCollector::handle(const ChunkPacket& pkt) {
    if (pkt.total_chunks == 0 || pkt.chunk_id >= pkt.total_chunks)
        return;

    auto& frame = frame_buffer[pkt.frame_id];

    // İlk defa geldiyse vektörü ayarla
    if (frame.chunks.empty()) {
        frame.chunks.resize(pkt.total_chunks);
        frame.received_flags.resize(pkt.total_chunks, false);
        frame.total_chunks = pkt.total_chunks;
    }

    // Duplicate kontrolü
    if (frame.received_flags[pkt.chunk_id])
        return;

    frame.chunks[pkt.chunk_id] = pkt.payload;
    frame.received_flags[pkt.chunk_id] = true;
    frame.received_chunks++;
    frame.last_update = Clock::now();

    // Tamamlanmış mı?
    if (frame.received_chunks == frame.total_chunks) {
        std::vector<uint8_t> assembled;
        for (const auto& chunk : frame.chunks)
            assembled.insert(assembled.end(), chunk.begin(), chunk.end());

        callback(assembled);
        frame_buffer.erase(pkt.frame_id);
    }
}

// Bu fonksiyon, receiver içinde periyodik olarak çağrılmalı
void SmartFrameCollector::flush_expired_frames() {
    TimePoint now = Clock::now();

    std::vector<uint16_t> to_finalize;
    for (auto& [fid, frame] : frame_buffer) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - frame.last_update).count();
        if (frame.received_chunks > 0 && elapsed > JITTER_TIMEOUT_MS) {
            to_finalize.push_back(fid);
        }
    }

    for (uint16_t fid : to_finalize) {
        auto& frame = frame_buffer[fid];
        std::vector<uint8_t> assembled;
        for (const auto& chunk : frame.chunks)
            assembled.insert(assembled.end(), chunk.begin(), chunk.end());

        callback(assembled);
        frame_buffer.erase(fid);
    }
}
