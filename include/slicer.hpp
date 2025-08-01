#pragma once
#include <vector>
#include <cstdint>

struct Chunk {
    uint32_t frame_id;
    uint16_t chunk_id;
    uint16_t total_chunks;
    std::vector<uint8_t> payload;
};

// Slice frame data into k chunks (no parity)
std::vector<Chunk> slice_frame(const std::vector<uint8_t>& frame_data,
                               uint32_t frame_id,
                               uint16_t chunk_size);

