#pragma once

#include "packet_parser.hpp"  // ChunkPacket burada tanımlı
#include <vector>
#include <string>
#include <cstddef>

// UDP soketlerini açar (bind gerekmez, client için)
bool init_udp_sockets(size_t count);

// Soketleri kapatır
void close_udp_sockets();

// Bir ChunkPacket'i hedef IP ve port'a gönder
void send_udp(const std::string& target_ip, int port, const ChunkPacket& packet);

