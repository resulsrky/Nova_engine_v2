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
    explicit WeightedScheduler(const std::vector<PathStats>& paths);
    PathStats select_path();
    void update_metrics(const std::vector<PathStats>& paths);

private:
    std::vector<PathStats> paths_;
    std::vector<int> cumulative_weights_;
    int total_weight = 0;
    std::mt19937 rng;

    void build_weight_table();
};

// 🔥 Global olarak seçimi kolaylaştıran interface
void init_scheduler(const std::vector<int>& ports); // Basit RTT bilmeden başlatmak için
int select_port_for_chunk(int chunk_id);
