#include "slicer.hpp"
#include <cstring>
#include <algorithm>

std::vector<Chunk> slice_frame(const uint8_t* frame_data, size_t frame_size, uint16_t frame_id) {
    std::vector<Chunk> chunks;

    if (!frame_data || frame_size == 0) return chunks;

    size_t payload_per_chunk = MAX_CHUNK_SIZE;
    size_t total_chunks = (frame_size + payload_per_chunk - 1) / payload_per_chunk;

    for (size_t i = 0; i < total_chunks; ++i) {
        size_t offset = i * payload_per_chunk;
        size_t remaining = frame_size - offset;
        size_t current_size = std::min(remaining, payload_per_chunk);

        Chunk chunk;
        chunk.data.resize(HEADER_SIZE + current_size);

        // Header (Big-endian frame_id)
        chunk.data[0] = static_cast<uint8_t>((frame_id >> 8) & 0xFF);
        chunk.data[1] = static_cast<uint8_t>(frame_id & 0xFF);
        chunk.data[2] = static_cast<uint8_t>(i);
        chunk.data[3] = static_cast<uint8_t>(total_chunks);

        // Payload
        std::memcpy(chunk.data.data() + HEADER_SIZE, frame_data + offset, current_size);

        chunks.push_back(std::move(chunk));
    }

    return chunks;
}
