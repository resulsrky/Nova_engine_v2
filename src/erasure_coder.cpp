#include "erasure_coder.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdint>

ErasureCoder::ErasureCoder(int k, int r, int w)
    : k_(k), r_(r), w_(w), bitmatrix_(nullptr) {}

void ErasureCoder::encode(const std::vector<std::vector<uint8_t>>& k_chunks,
                          std::vector<std::vector<uint8_t>>& out_blocks) {
    if (k_chunks.size() != static_cast<size_t>(k_))
        throw std::runtime_error("Invalid number of data chunks");

    size_t block_size = k_chunks[0].size();
    out_blocks = k_chunks; // k tane doğrudan veri bloğu

    // Dummy parity: k_chunks[0] xor k_chunks[1] xor ... → parity[i]
    for (int p = 0; p < r_; ++p) {
        std::vector<uint8_t> parity(block_size, 0);
        for (int i = 0; i < k_; ++i) {
            for (size_t j = 0; j < block_size; ++j) {
                parity[j] ^= k_chunks[i][j]; // dummy parity
            }
        }
        out_blocks.push_back(parity);
    }
}

bool ErasureCoder::decode(const std::vector<std::vector<uint8_t>>& recv_chunks,
                          std::vector<uint8_t>& recovered_data) {
    if (recv_chunks.size() < static_cast<size_t>(k_)) return false;

    // Dummy: sadece ilk k taneyi al, birleştir
    recovered_data.clear();
    for (int i = 0; i < k_; ++i)
        recovered_data.insert(recovered_data.end(),
                              recv_chunks[i].begin(), recv_chunks[i].end());
    return true;
}

