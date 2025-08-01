#pragma once

#include <vector>
#include <string>
#include <cstdint>

// Başlatma: hedef IP ve port listesi
void init_chunk_dispatcher(const std::string& target_ip, const std::vector<int>& ports);

// Raw bir UDP chunk verisini uygun porta yönlendir
void dispatch_chunk(const uint8_t* data, size_t size);
