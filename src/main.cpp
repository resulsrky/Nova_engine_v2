#include "sender_receiver.hpp"
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>

static volatile bool should_exit = false;

void signal_handler(int sig) {
    std::cout << "\nReceived signal " << sig << ", shutting down gracefully..." << std::endl;
    should_exit = true;
}

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <my_ip> <my_port> <remote_ip> <remote_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 192.168.1.5 45000 192.168.1.10 45001" << std::endl;
        std::cerr << "Each side uses different ports for bidirectional communication" << std::endl;
        return 1;
    }

    // Signal handler setup
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::string my_ip = argv[1];
    int my_port = std::stoi(argv[2]);
    std::string remote_ip = argv[3];
    int remote_port = std::stoi(argv[4]);

    std::cout << "Starting NovaEngine in bidirectional mode with fork..." << std::endl;
    std::cout << "My IP: " << my_ip << ":" << my_port << std::endl;
    std::cout << "Remote IP: " << remote_ip << ":" << remote_port << std::endl;

    // Her taraf kendi portunu dinliyor, karşı tarafın portuna gönderiyor
    std::vector<int> my_ports = {my_port};
    std::vector<int> remote_ports = {remote_port};
    
    // Fork ile ayrı process'lerde çalıştır
    pid_t sender_pid = fork();
    
    if (sender_pid == 0) {
        // Child process - Sender
        std::cout << "[SENDER] Process started (PID: " << getpid() << ")" << std::endl;
        
        // Set signal handler for child
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        try {
            run_sender(remote_ip, remote_ports);
        } catch (const std::exception& e) {
            std::cerr << "[SENDER] Exception: " << e.what() << std::endl;
        }
        
        std::cout << "[SENDER] Process finished" << std::endl;
        exit(0);
    } else if (sender_pid > 0) {
        // Parent process - Receiver
        std::cout << "[RECEIVER] Process started (PID: " << getpid() << ")" << std::endl;
        std::cout << "[RECEIVER] Sender PID: " << sender_pid << std::endl;
        
        try {
            run_receiver(my_ports);
        } catch (const std::exception& e) {
            std::cerr << "[RECEIVER] Exception: " << e.what() << std::endl;
        }
        
        // Wait for sender to finish
        int status;
        waitpid(sender_pid, &status, 0);
        std::cout << "[RECEIVER] Sender process finished" << std::endl;
    } else {
        // Fork failed
        std::cerr << "Fork failed!" << std::endl;
        return 1;
    }
    
    return 0;
}
