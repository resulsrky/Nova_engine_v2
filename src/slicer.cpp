#include "slicer.hpp"
#include <algorithm>

std::vector<Chunk> slice_frame(const std::vector<uint8_t>& frame_data,
                               uint32_t frame_id,
                               uint16_t chunk_size) {
    std::vector<Chunk> chunks;

    if (frame_data.empty() || chunk_size == 0) return chunks;

    size_t total_chunks = (frame_data.size() + chunk_size - 1) / chunk_size;

    for (size_t i = 0; i < total_chunks; ++i) {
        size_t offset = i * chunk_size;
        size_t len = std::min(static_cast<size_t>(chunk_size), frame_data.size() - offset);

        Chunk c;
        c.frame_id = frame_id;
        c.chunk_id = static_cast<uint16_t>(i);
        c.total_chunks = static_cast<uint16_t>(total_chunks);
        c.payload.insert(c.payload.end(), frame_data.begin() + offset, frame_data.begin() + offset + len);

        chunks.push_back(std::move(c));
    }

    return chunks;
}

