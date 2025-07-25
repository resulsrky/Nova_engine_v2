// include/slicer.h
#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

constexpr size_t MAX_CHUNK_SIZE = 1000;
constexpr size_t HEADER_SIZE = 4;

struct Chunk {
    std::vector<uint8_t> data;
};

std::vector<Chunk> slice_frame(const uint8_t* frame_data, size_t frame_size, uint16_t frame_id);
