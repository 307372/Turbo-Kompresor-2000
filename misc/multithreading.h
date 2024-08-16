#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <cmath>
#include <bitset>
#include <vector>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>

#include "integrity_validation.h"
#include "compression.h"

namespace multithreading
{
    using Flagset = std::bitset<16>;

    enum class mode : int
    {
        compress=100,
        decompress=200
    };

    inline uint16_t calculate_progress(float current, float whole);

    void processing_worker(
        multithreading::mode task,
        Compression* comp,
        uint16_t flags,
        bool& aborting_var,
        bool* is_finished,
        uint16_t* progress_ptr = nullptr);

    void processing_scribe(
        multithreading::mode task,
        std::fstream& output,
        std::vector<Compression*>& comp_v,
        bool worker_finished[],
        uint32_t block_count,
        uint64_t* compressed_size,
        std::string& checksum,
        bool& checksum_done,
        uint64_t original_size,
        bool& aborting_var,
        bool* successful,
        std::condition_variable& cond,
        std::mutex& cond_mut);

    bool processing_foreman(
        std::fstream &archive_stream,
        const std::string& target_path,
        multithreading::mode task,
        uint16_t flags,
        uint64_t original_size,
        uint64_t* compressed_size,
        bool& aborting_var,
        bool validate_integrity,
        uint16_t* progress_ptr,
        uint8_t* metadata=nullptr,
        uint32_t metadata_size=0);
}
#endif // MULTITHREADING_H
