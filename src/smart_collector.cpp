#include "smart_collector.hpp"
#include <chrono>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
constexpr int JITTER_TIMEOUT_MS = 15;

SmartFrameCollector::SmartFrameCollector(FrameReadyCallback cb, int k, int r)
    : callback(std::move(cb)), fec(k, r), k_(k), r_(r) {
    timeout_ms_ = JITTER_TIMEOUT_MS;
    flush_thread = std::thread([this]() {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms_ / 2));
            flush_expired_frames();
        }
    });
}

SmartFrameCollector::~SmartFrameCollector() {
    stop_flag = true;
    if (flush_thread.joinable()) flush_thread.join();
}

void SmartFrameCollector::handle(ChunkPacket pkt) {
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

    frame.chunks[pkt.chunk_id] = std::move(pkt.payload);
    frame.received_flags[pkt.chunk_id] = true;
    frame.received_chunks++;
    frame.last_update = Clock::now();

    // Yeterli chunk geldiyse FEC ile decode et
    if (frame.received_chunks >= static_cast<size_t>(k_)) {
        std::vector<uint8_t> recovered;
        if (fec.decode(frame.chunks, frame.received_flags, recovered)) {
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
        std::vector<uint8_t> recovered;
        if (fec.decode(frame.chunks, frame.received_flags, recovered)) {
            callback(recovered);
        } else {
            std::cerr << "[WARN] FEC decode failed for frame_id " << fid << "\n";
        }

        frame_buffer.erase(fid);
    }
}

