#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>

std::string target_ip = "192.168.1.12";  // hedef cihazın IP'si
std::vector<int> target_ports = {45094, 44824, 33061, 55008};

int main() {
    // sender thread'i başlat
    std::thread sender_thread([&]() {
        int dummy_argc = 1 + target_ports.size() + 1;
        std::vector<std::string> args = {"sender", target_ip};
        for (int port : target_ports)
            args.push_back(std::to_string(port));

        std::vector<char*> argv_c;
        for (auto& arg : args)
            argv_c.push_back(arg.data());

        run_sender(argv_c.size(), argv_c.data());
    });

    // receiver thread'i başlat
    std::thread receiver_thread(run_receiver);

    sender_thread.join();
    receiver_thread.join();

    return 0;
}
