#include "scheduler.hpp"
#include <numeric>
#include <stdexcept>

PathStats::PathStats(const std::string& ip, int port, double rtt, double loss)
    : ip(ip), port(port), rtt_ms(rtt), loss_ratio(loss) {
    double score = 1000.0 / (rtt + 1.0) * (1.0 - loss);
    weight = std::max(1, static_cast<int>(score));
}

WeightedScheduler::WeightedScheduler(const std::vector<PathStats>& paths)
    : paths_(paths), rng(std::random_device{}()) {
    build_weight_table();
}

void WeightedScheduler::build_weight_table() {
    cumulative_weights_.clear();
    total_weight = 0;

    for (const auto& path : paths_) {
        total_weight += path.weight;
        cumulative_weights_.push_back(total_weight);
    }
}

PathStats WeightedScheduler::select_path() {
    if (paths_.empty()) throw std::runtime_error("No paths available");

    std::uniform_int_distribution<int> dist(1, total_weight);
    int r = dist(rng);

    for (size_t i = 0; i < cumulative_weights_.size(); ++i) {
        if (r <= cumulative_weights_[i]) return paths_[i];
    }

    return paths_.back();
}

