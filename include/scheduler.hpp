#pragma once

#include <string>
#include <vector>
#include <random>

struct PathStats {
    std::string ip;
    int port;
    double rtt_ms;
    double loss_ratio;
    int weight;

    PathStats(const std::string& ip, int port, double rtt, double loss);
};

class WeightedScheduler {
public:
    WeightedScheduler(const std::vector<PathStats>& paths);

    // Rastgele path seçer, ağırlıklara göre
    PathStats select_path();

private:
    std::vector<PathStats> paths_;
    std::vector<int> cumulative_weights_;
    std::mt19937 rng;
    int total_weight;

    void build_weight_table();
};

