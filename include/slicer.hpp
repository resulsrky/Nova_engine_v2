#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

constexpr size_t MAX_CHUNK_SIZE = 1000;    // Payload boyutu
constexpr size_t HEADER_SIZE = 4;          // frame_id(2) + chunk_id(1) + total_chunks(1)

struct Chunk {
    std::vector<uint8_t> data;  // Header + Payload birleşik halde
};

// Frame verisini küçük parçalara ayırır
std::vector<Chunk> slice_frame(const uint8_t* frame_data, size_t frame_size, uint16_t frame_id);
