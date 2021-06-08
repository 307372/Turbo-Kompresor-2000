#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <cmath>
#include <bitset>
#include <vector>
#include <cassert>
#include <thread>
#include <sstream>

#include "integrity_validation.h"
#include "compression.h"

namespace multithreading
{
    enum class mode : int { compress=100, decompress=200 };

    inline uint16_t calculate_progress( float current, float whole );

    void processing_worker( const int task, Compression* comp, uint16_t flags, bool& aborting_var, bool* is_finished,
                            uint8_t*& key, uint8_t*& metadata, uint32_t& metadata_size, uint16_t* progress_ptr = nullptr );

    void processing_scribe( const int task, std::fstream& output, std::vector<Compression*>& comp_v,
                            bool worker_finished[], uint32_t block_count, uint64_t* compressed_size,
                            std::string& checksum, bool& checksum_done, uint64_t original_size, bool& aborting_var, bool* successful );

    bool processing_foreman( std::fstream &archive_stream, const std::string& target_path, multithreading::mode task, uint16_t flags,
                             uint64_t original_size, uint64_t* compressed_size, bool& aborting_var, bool validate_integrity,
                             uint16_t* progress_ptr, uint8_t** key=nullptr, uint8_t* metadata=nullptr, uint32_t metadata_size=0);
}
#endif // MULTITHREADING_H
