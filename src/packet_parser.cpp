#include "packet_parser.hpp"
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
// Paket formatı:
// [0-1]   frame_id      (2 byte)
// [2]     chunk_id      (1 byte)
// [3]     total_chunks  (1 byte)
// [4...]  payload       (kalan veri)

std::vector<uint8_t> serialize_packet(const ChunkPacket& pkt) {
    std::vector<uint8_t> buffer;
    buffer.reserve(4 + pkt.payload.size());

    // frame_id (2 byte - little endian)
    buffer.push_back(pkt.frame_id & 0xFF);
    buffer.push_back((pkt.frame_id >> 8) & 0xFF);

    // chunk_id (1 byte)
    buffer.push_back(pkt.chunk_id);

    // total_chunks (1 byte)
    buffer.push_back(pkt.total_chunks);

    // payload
    buffer.insert(buffer.end(), pkt.payload.begin(), pkt.payload.end());

    return buffer;
}

ChunkPacket parse_packet(const uint8_t* data, size_t len) {
    if (len < 4) {
        throw std::runtime_error("[parse_packet] Paket çok kısa!");
    }

    ChunkPacket pkt;
    pkt.frame_id = data[0] | (data[1] << 8);  // Little endian
    pkt.chunk_id = data[2];
    pkt.total_chunks = data[3];

    pkt.payload.assign(data + 4, data + len);
    return pkt;
}
