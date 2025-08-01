#ifndef NOVAENGINE_RTT_MONITOR_HPP
#define NOVAENGINE_RTT_MONITOR_HPP

#include <unordered_map>
#include <chrono>
#include <vector>
#include <mutex>
#include <deque>

class RTTMonitor {
public:
    static constexpr size_t MAX_HISTORY_SIZE = 10;
    
    RTTMonitor();

    void send_ping(int port);              // Ping gönder
    void receive_pong(int port);           // Pong geldiğinde RTT hesapla
    double get_rtt(int port);              // RTT sorgula
    std::vector<int> get_sorted_ports();   // RTT'ye göre portları sırala
    
    // Enhanced interface
    void startPing(int port, int64_t timestamp);
    void receivePong(int port, int64_t timestamp);
    double getRTT(int port);
    double getAverageRTT();

private:
    std::unordered_map<int, std::chrono::steady_clock::time_point> ping_sent_time;
    std::unordered_map<int, double> rtt_map;
    std::unordered_map<int, int64_t> ping_timestamps;
    std::unordered_map<int, std::deque<double>> rtt_history;
    std::mutex monitor_mutex;
};

#endif // NOVAENGINE_RTT_MONITOR_HPP
