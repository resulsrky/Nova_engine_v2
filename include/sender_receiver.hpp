#pragma once

#include <string>
#include <vector>


void run_sender(const std::string& public_ip, const std::vector<int>& ports);


void run_receiver(const std::vector<int>& ports);
