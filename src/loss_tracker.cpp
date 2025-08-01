#include "loss_tracker.hpp"
#include <chrono>
#include <algorithm>

using namespace std::chrono;

LossTracker::LossTracker(int timeout_ms) : timeout_ms_(timeout_ms) {}

void LossTracker::ping_sent(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    ping_map_[port].sent_time = steady_clock::now();
    ping_map_[port].waiting_response = true;
}

void LossTracker::pong_received(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    ping_map_[port].waiting_response = false;
}

void LossTracker::packetSent(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = port_stats_[port];
    stats.packets_sent++;
    stats.last_sent_time = steady_clock::now();
}

void LossTracker::packetReceived(int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = port_stats_[port];
    stats.packets_received++;
    stats.last_received_time = steady_clock::now();
}

void LossTracker::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = steady_clock::now();

    for (auto& [port, info] : ping_map_) {
        if (info.waiting_response) {
            auto elapsed = duration_cast<milliseconds>(now - info.sent_time).count();
            if (elapsed > timeout_ms_) {
                info.loss_count++;
                info.waiting_response = false;  // Tek sayılır
            }
        }
    }
}

int LossTracker::get_loss_count(int port) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ping_map_.find(port);
    return (it != ping_map_.end()) ? it->second.loss_count : 0;
}

double LossTracker::getLossRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (port_stats_.empty()) return 0.0;
    
    uint64_t total_sent = 0;
    uint64_t total_received = 0;
    
    for (const auto& [port, stats] : port_stats_) {
        total_sent += stats.packets_sent;
        total_received += stats.packets_received;
    }
    
    if (total_sent == 0) return 0.0;
    
    return static_cast<double>(total_sent - total_received) / total_sent;
}

double LossTracker::getLossRate(int port) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = port_stats_.find(port);
    if (it == port_stats_.end() || it->second.packets_sent == 0) {
        return 0.0;
    }
    
    return static_cast<double>(it->second.packets_sent - it->second.packets_received) 
           / it->second.packets_sent;
}

std::vector<int> LossTracker::get_high_loss_ports(int threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> result;

    for (const auto& [port, info] : ping_map_) {
        if (info.loss_count >= threshold)
            result.push_back(port);
    }

    return result;
}

std::vector<int> LossTracker::getHighLossPorts(double threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> result;

    for (const auto& [port, stats] : port_stats_) {
        if (stats.packets_sent > 0) {
            double loss_rate = static_cast<double>(stats.packets_sent - stats.packets_received) 
                              / stats.packets_sent;
            if (loss_rate > threshold) {
                result.push_back(port);
            }
        }
    }

    return result;
}
