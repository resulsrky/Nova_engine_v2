#pragma once
#include <map>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <functional>
#include <iostream>

#include "packet_parser.hpp"

class SmartFrameCollector {
public:
    using FrameHandler = std::function<void(const std::vector<uint8_t>&)>;

    explicit SmartFrameCollector(FrameHandler handler)
        : on_frame_ready(std::move(handler)) {}

    void handle(const ChunkPacket& pkt) {
        auto& frame = buffer[pkt.frame_id];
        frame.last_update = std::chrono::steady_clock::now();
        frame.total_chunks = pkt.total_chunks;
        frame.chunks[pkt.chunk_id] = pkt.payload;

        if (frame.chunks.size() == pkt.total_chunks) {
            flush(pkt.frame_id);
            return;
        }

        if (!frame.waiting) {
            frame.waiting = true;
            std::thread([this, fid = pkt.frame_id]() {
                std::this_thread::sleep_for(wait_time);
                auto it = buffer.find(fid);
                if (it == buffer.end()) return;

                auto now = std::chrono::steady_clock::now();
                if (now - it->second.last_update >= wait_time) {
                    flush(fid);
                }
            }).detach();
        }
    }

private:
    struct FrameState {
        std::map<uint8_t, std::vector<uint8_t>> chunks;
        std::chrono::steady_clock::time_point last_update;
        uint8_t total_chunks = 0;
        bool waiting = false;
    };

    std::unordered_map<uint16_t, FrameState> buffer;
    FrameHandler on_frame_ready;
    const std::chrono::milliseconds wait_time{20};

    void flush(uint16_t fid) {
        auto it = buffer.find(fid);
        if (it == buffer.end()) return;

        auto& chunks = it->second.chunks;
        std::vector<uint8_t> frame;
        for (int i = 0; i < it->second.total_chunks; ++i) {
            if (chunks.count(i))
                frame.insert(frame.end(), chunks[i].begin(), chunks[i].end());
            else
                std::cerr << "Eksik chunk " << i << " in frame " << fid << "\n";
        }

        on_frame_ready(frame);
        buffer.erase(fid);
    }
};
