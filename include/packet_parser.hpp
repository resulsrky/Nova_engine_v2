#pragma once
#include <vector>
#include <cstdint>

struct ChunkPacket {
    uint16_t frame_id;
    uint8_t chunk_id;
    uint8_t total_chunks;
    std::vector<uint8_t> payload;
};

inline ChunkPacket parse_packet(const uint8_t* data, size_t len) {
    ChunkPacket pkt;
    pkt.frame_id = (data[0] << 8) | data[1];
    pkt.chunk_id = data[2];
    pkt.total_chunks = data[3];
    pkt.payload.assign(data + 4, data + len);
    return pkt;
}
