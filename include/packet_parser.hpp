#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

struct ChunkPacket {
    uint16_t frame_id;
    uint8_t chunk_id;
    uint8_t total_chunks;
    std::vector<uint8_t> payload;
};

// Gelen UDP verisini çözümleyip bir ChunkPacket'e çevirir
inline ChunkPacket parse_packet(const uint8_t* data, size_t len) {
    ChunkPacket pkt;

    if (len < 4) {
        // Header bile yoksa boş paket dön
        pkt.frame_id = 0;
        pkt.chunk_id = 0;
        pkt.total_chunks = 0;
        return pkt;
    }

    pkt.frame_id = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    pkt.chunk_id = data[2];
    pkt.total_chunks = data[3];
    pkt.payload.assign(data + 4, data + len);

    return pkt;
}
