#pragma once

#include "packet_parser.hpp"  // ChunkPacket burada tanımlı
#include <vector>
#include <string>

// UDP soketlerini açar ve belirtilen yerel portlara bind eder
bool init_udp_sockets(const std::vector<int>& local_ports);

// Soketleri kapatır
void close_udp_sockets();

void set_target_addresses(const std::string& target_ip, const std::vector<int>& ports);

// Bir ChunkPacket'i hedef IP ve port'a gönder
// Non-blocking send; dönen byte sayısı <0 ise hata
ssize_t send_udp(const std::string& target_ip, int port, const ChunkPacket& packet);
ssize_t send_udp_multipath(const std::string& target_ip, const std::vector<int>& ports, const ChunkPacket& packet);

