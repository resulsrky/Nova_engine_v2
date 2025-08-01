#pragma once
#include <cstdint>
#include <vector>
#include <cstddef>    // <-- burada

struct ChunkPacket {
    uint16_t frame_id;
    uint8_t chunk_id;
    uint8_t total_chunks;
    int64_t timestamp;  // Microsecond timestamp for RTT calculation
    std::vector<uint8_t> payload;
};

// ChunkPacket → Byte array
std::vector<uint8_t> serialize_packet(const ChunkPacket& pkt);

// Byte array → ChunkPacket
ChunkPacket parse_packet(const uint8_t* data, std::size_t len);

