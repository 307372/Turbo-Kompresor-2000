#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include "integrity_validation.h"
#include "compression.h"

#include <cmath>
#include <bitset>
#include <vector>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <map>



enum class AlgorithmFlag : std::uint16_t
{
    BWT2 = 7,
    BWT = 0,
    MTF,
    RLE,
    AC,
    AC2
};





namespace multithreading
{
static std::map<std::string, AlgorithmFlag> strToAlgorithmFlag{
        {"BWT", AlgorithmFlag::BWT},
        {"BWT2", AlgorithmFlag::BWT2},
        {"MTF", AlgorithmFlag::MTF},
        {"RLE", AlgorithmFlag::RLE},
        {"AC", AlgorithmFlag::AC},
        {"AC2", AlgorithmFlag::AC2},
    }; 

// std::vector<AlgorithmFlag> compressionOrder{
//     AlgorithmFlag::BWT,
//     AlgorithmFlag::BWT2,
//     AlgorithmFlag::MTF,
//     AlgorithmFlag::RLE,
//     AlgorithmFlag::AC,
//     AlgorithmFlag::AC2};


// static std::vector<AlgorithmFlag> decompressionOrder{
//     AlgorithmFlag::AC2,
//     AlgorithmFlag::AC,
//     AlgorithmFlag::RLE,
//     AlgorithmFlag::MTF,
//     AlgorithmFlag::BWT2,
//     AlgorithmFlag::BWT};

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
