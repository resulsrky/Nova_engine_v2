#include "scheduler.hpp"
#include <numeric>
#include <stdexcept>
#include <iostream>

// ---------------------- PathStats ----------------------

PathStats::PathStats(const std::string& ip, int port, double rtt, double loss)
    : ip(ip), port(port), rtt_ms(rtt), loss_ratio(loss) {
    double score = 1000.0 / (rtt + 1.0) * (1.0 - loss);
    weight = std::max(1, static_cast<int>(score));
}

// ---------------------- WeightedScheduler ----------------------

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

    std::cout << "[scheduler] Toplam Ağırlık: " << total_weight << "\n";
}

PathStats WeightedScheduler::select_path() {
    if (paths_.empty()) throw std::runtime_error("No paths available");

    std::uniform_int_distribution<int> dist(1, total_weight);
    int r = dist(rng);

    for (size_t i = 0; i < cumulative_weights_.size(); ++i) {
        if (r <= cumulative_weights_[i]) return paths_[i];
    }

    return paths_.back(); // güvenlik için
}

void WeightedScheduler::update_metrics(const std::vector<PathStats>& paths) {
    paths_ = paths;
    for (auto& p : paths_) {
        double score = 1000.0 / (p.rtt_ms + 1.0) * (1.0 - p.loss_ratio);
        p.weight = std::max(1, static_cast<int>(score));
    }
    build_weight_table();
}

// ---------------------- Global interface ----------------------

static WeightedScheduler* g_scheduler = nullptr;

void init_scheduler(const std::vector<int>& ports) {
    std::vector<PathStats> stats;

    for (int port : ports) {
        // Şimdilik RTT 50ms ve loss 0.0 varsayalım. Daha sonra RTTMonitor ve LossTracker'dan güncellenecek.
        stats.emplace_back("127.0.0.1", port, 50.0, 0.0);
    }

    if (g_scheduler) delete g_scheduler;
    g_scheduler = new WeightedScheduler(stats);

    std::cout << "[scheduler] Scheduler başlatıldı " << ports.size() << " port ile.\n";
}

int select_port_for_chunk(int chunk_id) {
    if (!g_scheduler) throw std::runtime_error("Scheduler not initialized");

    PathStats selected = g_scheduler->select_path();
    return selected.port;
}
