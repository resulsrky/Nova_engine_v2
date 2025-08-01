#include "chunk_dispatcher.hpp"
#include "scheduler.hpp"
#include "udp_sender.hpp"
#include "packet_parser.hpp"
#include <iostream>

static std::string g_target_ip;
static std::vector<int> g_ports;

void init_chunk_dispatcher(const std::string& target_ip, const std::vector<int>& ports) {
    g_target_ip = target_ip;
    g_ports = ports;
}

void dispatch_chunk(const uint8_t* data, size_t size) {
    try {
        ChunkPacket pkt = parse_packet(data, size); // parse_chunk değil → parse_packet!

        int selected_port = select_port_for_chunk(pkt.chunk_id); // scheduler logic'i
        send_udp(g_target_ip, selected_port, pkt);               // soketten gönder
    } catch (const std::exception& e) {
        std::cout << "[dispatcher] Hatalı paket: " << e.what() << "\n";
    }
}
