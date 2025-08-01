#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>

struct ChunkPacket {
    uint16_t frame_id;
    uint8_t chunk_id;
    uint8_t total_chunks;
    std::vector<uint8_t> payload;
};

// ChunkPacket → Byte array
std::vector<uint8_t> serialize_packet(const ChunkPacket& pkt);

// Byte array → ChunkPacket
ChunkPacket parse_packet(const uint8_t* data, size_t len);

