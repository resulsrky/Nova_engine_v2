#pragma once

#include "slicer.hpp"
#include <vector>
#include <string>

bool init_udp_sockets(size_t count);
void close_udp_sockets();

void send_chunks_to_ports(
    const std::vector<Chunk>& chunks,
    const std::string& target_ip,
    const std::vector<int>& target_ports
);
