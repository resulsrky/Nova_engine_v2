#pragma once
#include <vector>

class ErasureCoder {
public:
    ErasureCoder(int k, int r, int w = 8);
    void encode(const std::vector<std::vector<uint8_t>>& k_chunks,
                std::vector<std::vector<uint8_t>>& out_blocks); // size = k+r
    bool decode(const std::vector<std::vector<uint8_t>>& recv_chunks,
                std::vector<uint8_t>& recovered_data);
private:
    int k_, r_, w_;
    int* bitmatrix_;
};

