#include "smart_collector.hpp"
#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
constexpr int JITTER_TIMEOUT_MS = 15;

SmartFrameCollector::SmartFrameCollector(FrameReadyCallback cb, int k, int r)
    : callback(std::move(cb)), fec(k, r), k_(k), r_(r) {}

void SmartFrameCollector::handle(const ChunkPacket& pkt) {
    if (pkt.total_chunks == 0 || pkt.chunk_id >= pkt.total_chunks)
        return;

    auto& frame = frame_buffer[pkt.frame_id];

    // İlk defa geldiyse vektörleri hazırla
    if (frame.chunks.empty()) {
        frame.chunks.resize(pkt.total_chunks);
        frame.received_flags.resize(pkt.total_chunks, false);
        frame.total_chunks = pkt.total_chunks;
    }

    // Duplicate check
    if (frame.received_flags[pkt.chunk_id])
        return;

    frame.chunks[pkt.chunk_id] = pkt.payload;
    frame.received_flags[pkt.chunk_id] = true;
    frame.received_chunks++;
    frame.last_update = Clock::now();

    // Yeterli chunk geldiyse FEC ile decode et
    if (frame.received_chunks >= static_cast<size_t>(k_)) {
        std::vector<std::vector<uint8_t>> valid_chunks;
        for (size_t i = 0; i < frame.total_chunks; ++i) {
            if (frame.received_flags[i])
                valid_chunks.push_back(frame.chunks[i]);
        }

        std::vector<uint8_t> recovered;
        if (fec.decode(valid_chunks, recovered)) {
            callback(recovered);
            frame_buffer.erase(pkt.frame_id);
        }
    }
}

void SmartFrameCollector::flush_expired_frames() {
    TimePoint now = Clock::now();
    std::vector<uint16_t> to_finalize;

    for (auto& [fid, frame] : frame_buffer) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - frame.last_update).count();
        if (frame.received_chunks >= static_cast<size_t>(k_) && elapsed > JITTER_TIMEOUT_MS) {
            to_finalize.push_back(fid);
        }
    }

    for (uint16_t fid : to_finalize) {
        auto& frame = frame_buffer[fid];

        std::vector<std::vector<uint8_t>> valid_chunks;
        for (size_t i = 0; i < frame.total_chunks; ++i) {
            if (frame.received_flags[i])
                valid_chunks.push_back(frame.chunks[i]);
        }

        std::vector<uint8_t> recovered;
        if (fec.decode(valid_chunks, recovered)) {
            callback(recovered);
        } else {
            std::cerr << "[WARN] FEC decode failed for frame_id " << fid << "\n";
        }

        frame_buffer.erase(fid);
    }
}

