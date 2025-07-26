#pragma once

#include "slicer.hpp"
#include <vector>
#include <string>

// UDP soketlerini açar (bind gerekmez, client tarafı için)
bool init_udp_sockets(size_t count);

// Soketleri kapatır
void close_udp_sockets();

// Chunk'ları verilen IP + çoklu portlara yollar
void send_chunks_to_ports(
    const std::vector<Chunk>& chunks,
    const std::string& target_ip,
    const std::vector<int>& target_ports
);
