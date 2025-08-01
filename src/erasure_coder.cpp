#include "erasure_coder.hpp"
#include <stdexcept>
#include <jerasure.h>
#include <reed_sol.h>
#include <cstring>
#include <vector>
#include <iostream>

ErasureCoder::ErasureCoder(int k, int r, int w)
    : k_(k), r_(r), w_(w) {
    matrix_ = reed_sol_vandermonde_coding_matrix(k_, r_, w_);
    if (!matrix_) throw std::runtime_error("Failed to create coding matrix");
}

ErasureCoder::~ErasureCoder() {
    if (matrix_) free(matrix_);
}

void ErasureCoder::encode(const std::vector<std::vector<uint8_t>>& k_chunks,
                          std::vector<std::vector<uint8_t>>& out_blocks) {
    if (k_chunks.size() != static_cast<size_t>(k_))
        throw std::runtime_error("Invalid number of data chunks");

    size_t block_size = k_chunks[0].size();
    out_blocks.assign(k_chunks.begin(), k_chunks.end());
    out_blocks.resize(k_ + r_, std::vector<uint8_t>(block_size, 0));

    std::vector<uint8_t*> data_ptrs(k_);
    std::vector<uint8_t*> code_ptrs(r_);
    for (int i = 0; i < k_; ++i) data_ptrs[i] = const_cast<uint8_t*>(k_chunks[i].data());
    for (int i = 0; i < r_; ++i) code_ptrs[i] = out_blocks[k_ + i].data();

    jerasure_matrix_encode(k_, r_, w_, matrix_,
                           reinterpret_cast<char**>(data_ptrs.data()),
                           reinterpret_cast<char**>(code_ptrs.data()),
                           block_size);
}

bool ErasureCoder::decode(const std::vector<std::vector<uint8_t>>& blocks,
                          const std::vector<bool>& received,
                          std::vector<uint8_t>& recovered_data) {
    if (blocks.size() != static_cast<size_t>(k_ + r_)) {
        std::cerr << "[FEC] Invalid blocks size: " << blocks.size() << " != " << (k_ + r_) << std::endl;
        return false;
    }
    if (received.size() != blocks.size()) {
        std::cerr << "[FEC] Invalid received flags size: " << received.size() << " != " << blocks.size() << std::endl;
        return false;
    }

    size_t block_size = blocks[0].size();
    
    // Count received blocks
    int received_count = 0;
    for (bool r : received) if (r) received_count++;
    
    if (received_count < k_) {
        std::cerr << "[FEC] Not enough blocks received: " << received_count << " < " << k_ << std::endl;
        return false;
    }

    // Create working copies of the blocks
    std::vector<std::vector<uint8_t>> working_blocks = blocks;
    
    // Prepare data and parity pointers
    std::vector<uint8_t*> data_ptrs(k_);
    std::vector<uint8_t*> code_ptrs(r_);
    
    for (int i = 0; i < k_; ++i) {
        data_ptrs[i] = working_blocks[i].data();
    }
    for (int i = 0; i < r_; ++i) {
        code_ptrs[i] = working_blocks[k_ + i].data();
    }

    // Build erasure list
    std::vector<int> erasures;
    for (int i = 0; i < k_ + r_; ++i) {
        if (!received[i]) {
            erasures.push_back(i);
        }
    }
    erasures.push_back(-1); // terminator

    // If no erasures, just return the data blocks
    if (erasures.size() == 1) {
        recovered_data.clear();
        for (int i = 0; i < k_; ++i) {
            recovered_data.insert(recovered_data.end(), 
                                working_blocks[i].begin(), 
                                working_blocks[i].end());
        }
        return true;
    }

    // Perform Reed-Solomon decoding
    int ret = jerasure_matrix_decode(k_, r_, w_, matrix_, 0, erasures.data(),
                                     reinterpret_cast<char**>(data_ptrs.data()),
                                     reinterpret_cast<char**>(code_ptrs.data()),
                                     block_size);
    
    if (ret < 0) {
        std::cerr << "[FEC] Decode failed with error: " << ret << std::endl;
        return false;
    }

    // Extract recovered data
    recovered_data.clear();
    for (int i = 0; i < k_; ++i) {
        recovered_data.insert(recovered_data.end(), 
                            working_blocks[i].begin(), 
                            working_blocks[i].end());
    }
    
    return true;
}

