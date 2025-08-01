#pragma once
#include <vector>
#include <cstdint>

class ErasureCoder {
public:
    ErasureCoder(int k, int r, int w = 8);
    ~ErasureCoder();

    void encode(const std::vector<std::vector<uint8_t>>& k_chunks,
                std::vector<std::vector<uint8_t>>& out_blocks); // size = k+r

    bool decode(const std::vector<std::vector<uint8_t>>& blocks,
                const std::vector<bool>& received,
                std::vector<uint8_t>& recovered_data);
private:
    int k_, r_, w_;
    int* matrix_;
};

