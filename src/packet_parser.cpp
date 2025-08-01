#include "packet_parser.hpp"
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
// Paket formatı:
// [0-1]   frame_id      (2 byte)
// [2]     chunk_id      (1 byte)
// [3]     total_chunks  (1 byte)
// [4-11]  timestamp     (8 byte - int64_t)
// [12...] payload       (kalan veri)

std::vector<uint8_t> serialize_packet(const ChunkPacket& pkt) {
    std::vector<uint8_t> buffer;
    buffer.reserve(12 + pkt.payload.size());

    // frame_id (2 byte - little endian)
    buffer.push_back(pkt.frame_id & 0xFF);
    buffer.push_back((pkt.frame_id >> 8) & 0xFF);

    // chunk_id (1 byte)
    buffer.push_back(pkt.chunk_id);

    // total_chunks (1 byte)
    buffer.push_back(pkt.total_chunks);

    // timestamp (8 byte - little endian)
    for (int i = 0; i < 8; ++i) {
        buffer.push_back((pkt.timestamp >> (i * 8)) & 0xFF);
    }

    // payload
    buffer.insert(buffer.end(), pkt.payload.begin(), pkt.payload.end());

    return buffer;
}

ChunkPacket parse_packet(const uint8_t* data, size_t len) {
    if (len < 12) {
        throw std::runtime_error("[parse_packet] Paket çok kısa!");
    }

    ChunkPacket pkt;
    pkt.frame_id = data[0] | (data[1] << 8);  // Little endian
    pkt.chunk_id = data[2];
    pkt.total_chunks = data[3];

    // timestamp (8 byte - little endian)
    pkt.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        pkt.timestamp |= static_cast<int64_t>(data[4 + i]) << (i * 8);
    }

    pkt.payload.assign(data + 12, data + len);
    return pkt;
}
