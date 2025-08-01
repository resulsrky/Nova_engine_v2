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
    void update();                    // Zamanı aşan portları “kaybedilmiş” olarak say

    int get_loss_count(int port) const;
    std::vector<int> get_high_loss_ports(int threshold = 3) const;

private:
    struct PingInfo {
        std::chrono::steady_clock::time_point sent_time;
        int loss_count = 0;
        bool waiting_response = false;
    };

    mutable std::mutex mutex_;
    std::unordered_map<int, PingInfo> ping_map_;
    int timeout_ms_;
};

#endif // NOVAENGINE_LOSS_TRACKER_HPP
