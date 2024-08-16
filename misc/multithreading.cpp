#include "multithreading.h"

#include <cmath>
#include <bitset>
#include <vector>
#include <cassert>
#include <fstream>
#include <thread>
#include <sstream>
#include <random>
#include <filesystem>
#include <condition_variable>
#include <iostream>

#include "integrity_validation.h"
#include "compression.h"

namespace
{
using Flagset = std::bitset<16>;

uint32_t getWorkerThreadCount(int blockCount)
{
    uint32_t workerThreadCount = std::thread::hardware_concurrency();

    // "if value is not well defined or not computable, (std::thread::hardware_concurrency) returns 0" ~cppreference.com
    if (workerThreadCount == 0)
    {
        workerThreadCount = 2;
    }

    if (workerThreadCount > blockCount)
    {
        workerThreadCount = blockCount;
    }

    return workerThreadCount;
}

inline uint16_t calculate_progress( float finishedWork, float totalWork )
{
    return roundf(finishedWork*100 / totalWork);
}

inline void incrementProgressCtr(uint16_t* progressCounterPtr)
{
    if (progressCounterPtr != nullptr)
    {
        (*progressCounterPtr)++;
    }
}

enum class AlgorithmFlag : std::uint16_t
{
    BWT2 = 7,
    BWT = 0,
    MTF,
    RLE,
    AC,
    AC2,
    rANS,
};

inline void compressIfNeeded(
    Compression* comp,
    const AlgorithmFlag& algorithmFlag,
    const Flagset& flagset,
    bool& aborting_var,
    uint16_t* progressCounterPtr)
{
    const auto flagsetIndex = static_cast<std::uint16_t>(algorithmFlag);
    if (flagset[flagsetIndex] and !aborting_var)
    {
        switch(algorithmFlag)
        {
            case AlgorithmFlag::BWT:
            comp->BWT_make();
            break;
            case AlgorithmFlag::BWT2:
            comp->BWT_make2();
            break;
            case AlgorithmFlag::MTF:
            comp->MTF_make();
            break;
            case AlgorithmFlag::RLE:
            comp->RLE_makeV2();
            break;
            case AlgorithmFlag::AC:
            comp->AC_make();
            break;
            case AlgorithmFlag::AC2:
            comp->AC2_make();
            break;
            case AlgorithmFlag::rANS:
            comp->rANS_make();
            break;
        }
        incrementProgressCtr(progressCounterPtr);
    }
}

}

namespace multithreading
{
    void performCompression(
        Compression* comp,
        const Flagset& flagset,
        bool& aborting_var,
        uint16_t* progressCounterPtr)
    {
        static const std::vector<AlgorithmFlag> compressionOrder{
            AlgorithmFlag::BWT,
            AlgorithmFlag::BWT2,
            AlgorithmFlag::MTF,
            AlgorithmFlag::RLE,
            AlgorithmFlag::AC,
            AlgorithmFlag::AC2,
            AlgorithmFlag::rANS};

        for (auto algo : compressionOrder)
        {
            compressIfNeeded(
                comp,
                algo,
                flagset,
                aborting_var,
                progressCounterPtr);
        }
    }

inline void decompressIfNeeded(
    Compression* comp,
    const AlgorithmFlag& algorithmFlag,
    const Flagset& flagset,
    bool& aborting_var,
    uint16_t* progressCounterPtr)
{
    const auto flagsetIndex = static_cast<std::uint16_t>(algorithmFlag);
    if (flagset[flagsetIndex] and !aborting_var)
    {
        switch(algorithmFlag)
        {
            case AlgorithmFlag::BWT:
            comp->BWT_reverse();
            break;
            case AlgorithmFlag::BWT2:
            comp->BWT_reverse2();
            break;
            case AlgorithmFlag::MTF:
            comp->MTF_reverse();
            break;
            case AlgorithmFlag::RLE:
            comp->RLE_reverseV2();
            break;
            case AlgorithmFlag::AC:
            comp->AC_reverse();
            break;
            case AlgorithmFlag::AC2:
            comp->AC2_reverse();
            break;
            case AlgorithmFlag::rANS:
            comp->rANS_reverse();
            break;
        }
        incrementProgressCtr(progressCounterPtr);
    }
}

    void performDecompression(
        Compression* comp,
        const Flagset& flagset,
        bool& aborting_var,
        uint16_t* progressCounterPtr)
    {
        static const std::vector<AlgorithmFlag> decompressionOrder{
            AlgorithmFlag::rANS,
            AlgorithmFlag::AC2,
            AlgorithmFlag::AC,
            AlgorithmFlag::RLE,
            AlgorithmFlag::MTF,
            AlgorithmFlag::BWT2,
            AlgorithmFlag::BWT};
        for (auto algo : decompressionOrder)
        {
            decompressIfNeeded(
                comp,
                algo,
                flagset,
                aborting_var,
                progressCounterPtr);
        }
    }

    void processing_worker(
            multithreading::mode task,
            Compression* comp,
            uint16_t flags,
            bool& aborting_var,
            bool* is_finished,
            uint16_t* progress_ptr)
    {
        Flagset bin_flags = flags;
        if (task == multithreading::mode::compress)
        {
            performCompression(comp, bin_flags, aborting_var, progress_ptr);
        }
        else if (task == multithreading::mode::decompress)
        {
            performDecompression(comp, bin_flags, aborting_var, progress_ptr);
        }
        *is_finished = true;
    }

    void writeChecksumWhenReady(
        std::fstream& output,
        std::string& checksum,
        bool& checksum_done,
        bool& aborting_var,
        bool* successful,
        std::condition_variable& cond,
        std::unique_lock<std::mutex>& lock)
    {
        while (!checksum_done or aborting_var)
        {
            std::cout << "awaiting checksum" << std::endl;
            cond.wait(lock);
        }
        if (aborting_var) return;
        if (checksum.length() != 0)
        {
            output.write(checksum.c_str(), checksum.length());
        }
        *successful = true;
        std::cout << "Checksum done" << std::endl;
    }

    enum class ChecksumType
    {
        CRC32,
        SHA1,
        SHA256,
    };

    ChecksumType getChecksumTypeByLength(std::size_t length)
    {
        switch (length)
        {
            case 10:
                return ChecksumType::CRC32;
            case 40:
                return ChecksumType::SHA1;
            case 64:
                return ChecksumType::SHA256;
        }
    }

    std::string calculateChecksumFromDecompressedFile(
        ChecksumType type,
        std::fstream& output,
        bool& aborting_var,
        uint64_t original_size)
    {
        IntegrityValidation iv;
        switch (type)
        {
            case ChecksumType::CRC32:
                return iv.get_CRC32_from_stream(output, aborting_var);
            case ChecksumType::SHA1:
                return iv.get_SHA1_from_stream(output, original_size, aborting_var);
            case ChecksumType::SHA256:
                return iv.get_SHA256_from_stream(output, aborting_var);
        }
    }

    void validateChecksumWhenReady(
        std::fstream& output,
        std::string& checksum,
        bool& checksum_done,
        bool& aborting_var,
        uint64_t original_size,
        bool* successful,
        std::condition_variable& cond,
        std::unique_lock<std::mutex>& lock)
    {
        // awake after checksum is completed
        while (!checksum_done) cond.wait(lock);

        if (aborting_var) return;
        
        if (checksum.length() != 0)
        {
            ChecksumType checksumType = getChecksumTypeByLength(checksum.length());
            std::string recalculatedChecksum = calculateChecksumFromDecompressedFile(
                checksumType,
                output,
                aborting_var,
                original_size);

            *successful = recalculatedChecksum == checksum;
        }
        else *successful = true;
    }

    void writeBlockMetadata(
        std::uint32_t blockIndex,
        std::uint32_t blockSize, // comp_v[blockIndex]->size
        std::fstream& output,
        uint64_t* compressedSize)
    {
        std::stringstream blockMetadata;
        blockMetadata.write((char *) &blockIndex, sizeof(blockIndex));
        blockMetadata.write((char *) &blockSize, sizeof(blockSize));
        output << blockMetadata.rdbuf();

        constexpr std::size_t metadataSizeInBytes{8};
        *compressedSize += blockSize + metadataSizeInBytes;
    }

    void writeBlock(
        std::uint32_t blockIndex,
        std::fstream& output,
        std::vector<Compression*>& comp_v)
    {
        comp_v[blockIndex]->save_text(output);
        delete comp_v[blockIndex];
        comp_v[blockIndex] = nullptr;

        std::cout << "Block " << blockIndex << " saved" << std::endl;
    }

    void writeResults(
        multithreading::mode task,
            std::fstream& output,
            std::vector<Compression*>& comp_v,
            bool worker_finished[],
            uint32_t block_count,
            uint64_t* compressed_size,
            bool& aborting_var,
            std::condition_variable& cond,
            std::unique_lock<std::mutex>& lock)
    {
        uint32_t next_to_write = 0;  // index of last written block of data in comp_v
        while ( next_to_write != block_count )
        {
            if (aborting_var) return;

            if (worker_finished[next_to_write]) {
                if (task == multithreading::mode::compress) {
                    writeBlockMetadata(
                        next_to_write,
                        comp_v[next_to_write]->size,
                        output,
                        compressed_size);
                }

                writeBlock(next_to_write, output, comp_v);
                next_to_write++;
            }
            else {
                std::cout << "Waiting for block " << next_to_write << "to be finished" << std::endl;
                cond.wait(lock); // awake after each block is finished
            }
        }
    }

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
            std::mutex& cond_mut)
    {
        assert( output.is_open() );
        uint32_t next_to_write = 0;  // index of last written block of data in comp_v
        *compressed_size = 0;
        std::unique_lock<std::mutex> lock(cond_mut);
        while ( next_to_write != block_count )
        {
            if (aborting_var) return;

            if (worker_finished[next_to_write]) {
                if (task == multithreading::mode::compress) {
                    std::stringstream block_metadata;
                    block_metadata.write((char *) &next_to_write, sizeof(next_to_write)); // part number
                    block_metadata.write((char *) &comp_v[next_to_write]->size,
                                         sizeof(comp_v[next_to_write]->size));
                    output << block_metadata.rdbuf();
                    *compressed_size += comp_v[next_to_write]->size + 4 + 4;    // due to part number and block size
                }

                comp_v[next_to_write]->save_text(output);
                delete comp_v[next_to_write];
                comp_v[next_to_write] = nullptr;
                std::cout << "Block " << next_to_write << " saved" << std::endl;
                next_to_write++;
            }
            else {
                std::cout << "Waiting for block " << next_to_write << "to be finished" << std::endl;
                cond.wait(lock); // awake after each block is finished
            }
        }

        if (task == multithreading::mode::compress) {
            writeChecksumWhenReady(
                output,
                checksum,
                checksum_done,
                aborting_var,
                successful,
                cond,
                lock);
        }
        else if (task == multithreading::mode::decompress)
        {
            validateChecksumWhenReady(
                output,
                checksum,
                checksum_done,
                aborting_var,
                original_size,
                successful,
                cond,
                lock);

            if (output.is_open()) output.close();
        }
    }


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
            uint8_t* metadata,
            uint32_t metadata_size)
    /*/ 1. delegates work to each thread
    // 2. calculates checksum and sends it to scribe, after all the blocks of data have been processed

    // compression:
    // metadata is passed into this function

    // decompression:
    // metadata starts empty, and then is extracted from a file */
    {
        assert(task == mode::compress xor task == mode::decompress);
        assert(archive_stream.is_open());
        std::cout << target_path << std::endl;
        if (task == mode::compress)
            assert(std::filesystem::exists(target_path));

        const auto bin_flags = Flagset{flags};
        uint32_t block_size = 1 << 24;  // 2^24 Bytes = 16 MiB, default block size

        if ( bin_flags[9]  ) block_size >>= 1;
        if ( bin_flags[10] ) block_size >>= 2;
        if ( bin_flags[11] ) block_size >>= 4;
        if ( bin_flags[12] ) block_size >>= 8;

        uint32_t block_count = ceill((long double)original_size / block_size);
        if (block_count == 1) block_size = original_size;
        if (block_count == 0) {
            block_count = 1;
            block_size = 0;
        }

        // preparing vector of empty Compression objects for threads
        std::vector<Compression*> comp_v;
        for (uint32_t i=0; i < block_count; ++i) comp_v.emplace_back(new Compression(aborting_var));

        uint32_t worker_count = getWorkerThreadCount(block_count);

        std::fstream target_stream;
        if (task == multithreading::mode::compress)
        {
            target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
        }
        else if (task == multithreading::mode::decompress)
        {
            target_stream.open(target_path, std::ios::binary | std::ios::out);  // making sure target file exists
            target_stream.close();
            target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
        }

        assert(archive_stream.is_open());
        assert(target_stream.is_open());

        bool* task_finished_arr = new bool[block_count];
        bool* task_started_arr  = new bool[block_count];
        for (uint32_t i=0; i < block_count; ++i)
        {
            task_finished_arr[i] = false;
            task_started_arr[i] = false;
        }

        std::vector<std::thread> workers;
        std::string checksum;
        bool checksum_done = false;
        uint32_t lowest_free_work_ind = 0;

        // filling compression objects, and starting worker threads to process them
        for ( uint32_t i=0; i < worker_count; ++i )
        {
            if (aborting_var) break;
            if (task == multithreading::mode::compress)
            {
                comp_v[i]->load_part(target_stream, original_size, i, block_size);
                comp_v[i]->part_id = i;

                if (compressed_size != nullptr) *compressed_size = 0;
            }
            else if (task == multithreading::mode::decompress) {
                archive_stream.read((char*)&comp_v[i]->part_id, sizeof(comp_v[i]->part_id));
                archive_stream.read((char*)&comp_v[i]->size, sizeof(comp_v[i]->size));
                comp_v[i]->load_text(archive_stream, comp_v[i]->size);
            }

            workers.emplace_back(
                        &processing_worker,
                        task,
                        comp_v[i],
                        flags,
                        std::ref(aborting_var),
                        &task_finished_arr[i],
                        progress_ptr);


            task_started_arr[i] = true;
            lowest_free_work_ind++;
        }
        if (aborting_var)
        {
            for (auto& th: workers) th.join();
            delete[] task_finished_arr;
            delete[] task_started_arr;
            for (auto & comp : comp_v) delete comp;

            return false;
        }
        std::thread scribe;
        std::mutex scribe_mut;
        std::unique_lock<std::mutex> scribe_lock(scribe_mut);
        scribe_lock.unlock();
        std::condition_variable scribe_cond{};

        bool successful = false;

        if (task == multithreading::mode::compress)
        {
            scribe = std::thread(
                &processing_scribe,
                task,
                std::ref(archive_stream),
                std::ref(comp_v),
                task_finished_arr,
                block_count,
                compressed_size,
                std::ref(checksum),
                std::ref(checksum_done),
                original_size,
                std::ref(aborting_var),
                &successful,
                std::ref(scribe_cond),
                std::ref(scribe_mut));
        }
        else if (task == multithreading::mode::decompress)
            scribe = std::thread( &processing_scribe, task, std::ref(target_stream), std::ref(comp_v), task_finished_arr,
                                  block_count, compressed_size, std::ref(checksum), std::ref(checksum_done),
                                  original_size, std::ref(aborting_var), &successful, std::ref(scribe_cond), std::ref(scribe_mut));

        while (lowest_free_work_ind != block_count and !aborting_var) {

            for (uint32_t i=0; i < workers.size() and !aborting_var; ++i) {
                if (workers[i].joinable()) {
                    if (workers[i].joinable()) workers[i].join();
                    scribe_cond.notify_one();
                    if (aborting_var) break;

                    if (lowest_free_work_ind != block_count) {
                        if (task == multithreading::mode::compress) {
                            comp_v[lowest_free_work_ind]->load_part(target_stream, original_size, lowest_free_work_ind, block_size);
                            comp_v[lowest_free_work_ind]->part_id = lowest_free_work_ind;
                        }
                        else if (task == multithreading::mode::decompress) {
                            archive_stream.read((char*)&comp_v[lowest_free_work_ind]->part_id, sizeof(comp_v[lowest_free_work_ind]->part_id));
                            archive_stream.read((char*)&comp_v[lowest_free_work_ind]->size, sizeof(comp_v[lowest_free_work_ind]->size));
                            comp_v[lowest_free_work_ind]->load_text(archive_stream, comp_v[lowest_free_work_ind]->size);
                        }

                        workers.emplace_back(&processing_worker,
                                             task,
                                             comp_v[lowest_free_work_ind],
                                             flags,
                                             std::ref(aborting_var),
                                             &task_finished_arr[lowest_free_work_ind],
                                             progress_ptr);

                        lowest_free_work_ind++;
                    }
                }
            }
        }

        if (aborting_var)
        {
            scribe_cond.notify_one();
            for (auto& th: workers) if (th.joinable()) th.join();
            if (scribe.joinable()) scribe.join();
            delete[] task_finished_arr;
            delete[] task_started_arr;
            for (auto & comp : comp_v) delete comp;

            return false;
        }

        if (task == multithreading::mode::compress) {
            // since we're done with giving workers work, we can calculate checksum, which scribe thread will append to file

            if (bin_flags[15])  // SHA-1
            {
                IntegrityValidation iv;
                checksum = iv.get_SHA1_from_file(target_path, aborting_var);
            }
            if (bin_flags[14])  // CRC-32
            {
                IntegrityValidation iv;
                checksum = iv.get_CRC32_from_file(target_path, aborting_var);
            }
            if (bin_flags[13])  // SHA-256
            {
                IntegrityValidation iv;
                checksum = iv.get_SHA256_from_file(target_path, aborting_var);
            }

            checksum_done = true;
            scribe_cond.notify_one();
        }
        else if (task == multithreading::mode::decompress)
        {
            if (bin_flags[15])  // SHA-1
            {
                checksum = std::string(40, 0x00);
                archive_stream.read((char*)checksum.data(), checksum.length());
            }
            if (bin_flags[14])  // CRC-32
            {
                checksum = std::string(10, 0x00);
                archive_stream.read((char*)checksum.data(), checksum.length());
            }
            if (bin_flags[13])  // SHA-256
            {
                checksum = std::string(64, 0x00);
                archive_stream.read((char*)checksum.data(), checksum.length());
            }

            checksum_done = true;
            scribe_cond.notify_one();
        }

        for (auto& th : workers) if (th.joinable()) th.join();
        scribe_cond.notify_one();
        if (scribe.joinable()) scribe.join();

        delete[] task_finished_arr;
        delete[] task_started_arr;
        for (auto & comp : comp_v) delete comp;

        if (aborting_var) return false;
        return successful;
    }

}
