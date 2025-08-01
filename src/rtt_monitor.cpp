#include "rtt_monitor.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>

using namespace std::chrono;

RTTMonitor::RTTMonitor() {}

void RTTMonitor::send_ping(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    ping_sent_time[port] = steady_clock::now();
}

void RTTMonitor::startPing(int port, int64_t timestamp) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    ping_sent_time[port] = steady_clock::now();
    ping_timestamps[port] = timestamp;
}

void RTTMonitor::receive_pong(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);

    auto it = ping_sent_time.find(port);
    if (it != ping_sent_time.end()) {
        auto sent_time = it->second;
        auto now = steady_clock::now();
        auto rtt_ms = duration_cast<milliseconds>(now - sent_time).count();
        rtt_map[port] = static_cast<double>(rtt_ms);
        
        // Update RTT history for averaging
        rtt_history[port].push_back(static_cast<double>(rtt_ms));
        if (rtt_history[port].size() > MAX_HISTORY_SIZE) {
            rtt_history[port].pop_front();
        }
        
        // Opsiyonel: debug çıktısı
        std::cout << "[RTT] Port " << port << " RTT = " << rtt_ms << " ms\n";
    }
}

void RTTMonitor::receivePong(int port, int64_t timestamp) {
    std::lock_guard<std::mutex> lock(monitor_mutex);

    auto it = ping_timestamps.find(port);
    if (it != ping_timestamps.end()) {
        auto sent_timestamp = it->second;
        auto rtt_us = timestamp - sent_timestamp;
        auto rtt_ms = rtt_us / 1000.0;
        
        rtt_map[port] = rtt_ms;
        
        // Update RTT history for averaging
        rtt_history[port].push_back(rtt_ms);
        if (rtt_history[port].size() > MAX_HISTORY_SIZE) {
            rtt_history[port].pop_front();
        }
        
        ping_timestamps.erase(port);
    }
}

double RTTMonitor::get_rtt(int port) {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    auto it = rtt_map.find(port);
    return (it != rtt_map.end()) ? it->second : -1.0;  // -1.0 = bilinmiyor
}

double RTTMonitor::getRTT(int port) {
    return get_rtt(port);
}

double RTTMonitor::getAverageRTT() {
    std::lock_guard<std::mutex> lock(monitor_mutex);
    
    if (rtt_map.empty()) return -1.0;
    
    double sum = 0.0;
    int count = 0;
    
    for (const auto& [port, rtt] : rtt_map) {
        if (rtt > 0) {
            sum += rtt;
            count++;
        }
    }
    
    return count > 0 ? sum / count : -1.0;
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
