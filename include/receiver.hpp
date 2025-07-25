#pragma once
#include <vector>

/// Receiver thread'ini başlatır.
/// port_list: Dinlenecek portlar
void run_receiver(const std::vector<int>& port_list);
