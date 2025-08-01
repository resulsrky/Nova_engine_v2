#include "loss_tracker.hpp"
#include <chrono>

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

std::vector<int> LossTracker::get_high_loss_ports(int threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> result;

    for (const auto& [port, info] : ping_map_) {
        if (info.loss_count >= threshold)
            result.push_back(port);
    }

    return result;
}
