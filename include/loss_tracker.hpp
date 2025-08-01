#ifndef NOVAENGINE_LOSS_TRACKER_HPP
#define NOVAENGINE_LOSS_TRACKER_HPP

#include <unordered_map>
#include <chrono>
#include <mutex>
#include <vector>

class LossTracker {
public:
    LossTracker(int timeout_ms = 300); // Varsayılan 300ms

    void ping_sent(int port);         // Zamanı kaydet
    void pong_received(int port);     // Cevap alındı → başarısızlıktan çıkar
    void update();                    // Zamanı aşan portları "kaybedilmiş" olarak say

    int get_loss_count(int port) const;
    std::vector<int> get_high_loss_ports(int threshold = 3) const;
    
    // Enhanced packet loss tracking
    void packetSent(int port);
    void packetReceived(int port);
    double getLossRate() const;
    double getLossRate(int port) const;
    std::vector<int> getHighLossPorts(double threshold = 0.05) const;

private:
    struct PingInfo {
        std::chrono::steady_clock::time_point sent_time;
        int loss_count = 0;
        bool waiting_response = false;
    };
    
    struct PortStats {
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        std::chrono::steady_clock::time_point last_sent_time;
        std::chrono::steady_clock::time_point last_received_time;
    };

    mutable std::mutex mutex_;
    std::unordered_map<int, PingInfo> ping_map_;
    std::unordered_map<int, PortStats> port_stats_;
    int timeout_ms_;
};

#endif // NOVAENGINE_LOSS_TRACKER_HPP
