#ifndef NOVAENGINE_RTT_MONITOR_HPP
#define NOVAENGINE_RTT_MONITOR_HPP

#include <unordered_map>
#include <chrono>
#include <vector>
#include <mutex>

class RTTMonitor {
public:
    RTTMonitor();

    void send_ping(int port);              // Ping gönder
    void receive_pong(int port);           // Pong geldiğinde RTT hesapla
    double get_rtt(int port);              // RTT sorgula
    std::vector<int> get_sorted_ports();   // RTT’ye göre portları sırala

private:
    std::unordered_map<int, std::chrono::steady_clock::time_point> ping_sent_time;
    std::unordered_map<int, double> rtt_map;
    std::mutex monitor_mutex;
};

#endif // NOVAENGINE_RTT_MONITOR_HPP
