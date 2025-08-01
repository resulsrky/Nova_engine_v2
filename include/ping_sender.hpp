#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct PingChunk {
    uint32_t magic;           // 0xFEEDFACE
    uint64_t timestamp_ns;    // nanosecond timestamp
};

// Hedef IP ve portlara ping gönderir
void send_ping_chunks(const std::string& ip, const std::vector<int>& ports);

// Ping cevabını al ve RTT'yi hesapla
void listen_for_ping_response(int listen_port);

