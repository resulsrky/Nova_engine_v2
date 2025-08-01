#include "rtt_monitor.hpp"
#include <iostream>
#include <algorithm>

using namespace std::chrono;

RTTMonitor::RTTMonitor() {}

void RTTMonitor::send_ping(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    ping_sent_time[port] = steady_clock::now();
}

void RTTMonitor::receive_pong(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);

    auto it = ping_sent_time.find(port);
    if (it != ping_sent_time.end()) {
        auto sent_time = it->second;
        auto now = steady_clock::now();
        auto rtt_ms = duration_cast<milliseconds>(now - sent_time).count();
        rtt_map[port] = static_cast<double>(rtt_ms);
        // Opsiyonel: debug çıktısı
        std::cout << "[RTT] Port " << port << " RTT = " << rtt_ms << " ms\n";
    }
}

double RTTMonitor::get_rtt(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    auto it = rtt_map.find(port);
    return (it != rtt_map.end()) ? it->second : -1.0;  // -1.0 = bilinmiyor
}

std::vector<int> RTTMonitor::get_sorted_ports() {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    std::vector<std::pair<int, double>> pairs(rtt_map.begin(), rtt_map.end());

    std::sort(pairs.begin(), pairs.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });

    std::vector<int> sorted_ports;
    for (const auto& p : pairs)
        sorted_ports.push_back(p.first);

    return sorted_ports;
}
