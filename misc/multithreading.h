#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#include <cmath>
#include <bitset>
#include <vector>
#include <cassert>
#include <fstream>
#include <thread>
#include <sstream>

#include "integrity_validation.h"
#include "compression.h"

namespace multithreading
{
    inline uint16_t calculate_progress( float current, float whole ) { return roundf(current*100 / whole); }

    void processing_worker( const int task, Compression* comp, uint16_t flags, bool& aborting_var, bool* is_finished, uint16_t* progress_ptr = nullptr )
    {
        std::bitset<16> bin_flags = flags;
        if (task == Compression::compress) {
            if (bin_flags[0] and !aborting_var) {
                comp->BWT_make();
                if (progress_ptr != nullptr) (*progress_ptr)++;

            }

            if (bin_flags[1] and !aborting_var) {
                comp->MTF_make();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if (bin_flags[2] and !aborting_var) {
                comp->RLE_makeV2();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if (bin_flags[3] and !aborting_var) {
                comp->AC_make();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if (bin_flags[4] and !aborting_var) {
                comp->AC2_make();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }
        }
        else if (task == Compression::decompress)
        {
            if ( bin_flags[4] and !aborting_var) {
                comp->AC2_reverse();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if ( bin_flags[3] and !aborting_var) {
                comp->AC_reverse();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if ( bin_flags[2] and !aborting_var) {
                comp->RLE_reverseV2();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if ( bin_flags[1] and !aborting_var) {
                comp->MTF_reverse();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }

            if ( bin_flags[0] and !aborting_var ) {
                comp->BWT_reverse();
                if (progress_ptr != nullptr) (*progress_ptr)++;
            }
        }
        *is_finished = true;
    }


    void processing_scribe( const int task, std::fstream& output, std::vector<Compression*>& comp_v,
                            bool worker_finished[], uint32_t block_count, uint64_t* compressed_size,
                            std::string& checksum, bool& checksum_done, uint64_t original_size, bool& aborting_var, bool* successful )
    {
        assert( output.is_open() );
        uint32_t next_to_write = 0;  // index of last written block of data in comp_v
        *compressed_size = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while ( next_to_write != block_count )
        {
            if (aborting_var) return;

            if (worker_finished[next_to_write]) {
                if (task == Compression::compress) {
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
                next_to_write++;
            }
            else std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (task == Compression::compress) {
            while (!checksum_done or aborting_var) std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (aborting_var) return;
            if (checksum.length() != 0) output.write(checksum.c_str(), checksum.length());
            *successful = true; // if this didn't crash, then I guess it succeeded

        }
        else if (task == Compression::decompress)
        {
            while (!checksum_done) std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (aborting_var) return;
            if (checksum.length() != 0)
            {

                IntegrityValidation iv;
                std::string new_checksum;
                if ( checksum.length() == 10 )  // CRC-32
                {
                    new_checksum = iv.get_CRC32_from_stream(output, aborting_var);
                }
                else if ( checksum.length() == 40 ) // SHA-1
                {
                    new_checksum = iv.get_SHA1_from_stream(output, original_size, aborting_var);
                }
                if (new_checksum == checksum) {
                    *successful = true;
                }
                else {
                    *successful = false;
                }
            }
            else *successful = true;    // if the checksum is not checked, we assume success

        }
        if (output.is_open() and task == Compression::decompress) output.close();
    }


    bool processing_foreman( std::fstream &archive_stream, const std::string& target_path, const int task, uint16_t flags,
                             uint64_t original_size, uint64_t* compressed_size, bool& aborting_var, bool validate_integrity, uint16_t* progress_ptr )
    {
        // 1. delegates work to each thread
        // 2. calculates checksum and sends it to scribe, after all the blocks of data have been processed
        std::bitset<16> bin_flags = flags;
        uint32_t block_size = 1 << 24;  // 2^24 Bytes = 16 MiB, default block size

        if ( bin_flags[9] ) block_size >>= 1;
        if ( bin_flags[10] ) block_size >>= 2;
        if ( bin_flags[11] ) block_size >>= 4;
        if ( bin_flags[12] ) block_size >>= 8;



        uint32_t block_count = ceill((long double)original_size / block_size);
        if (block_count == 1) block_size = original_size;

        // preparing vector of empty Compression objects for threads
        std::vector<Compression*> comp_v;
        for (uint32_t i=0; i < block_count; ++i) comp_v.emplace_back(new Compression(aborting_var));


        uint32_t worker_count = std::thread::hardware_concurrency();
        // "if value is not well defined or not computable, (std::thread::hardware_concurrency) returns 0" ~cppreference.com
        if (worker_count == 0) worker_count = 2;
        if (worker_count > block_count) worker_count = block_count;

        std::fstream target_stream;
        if (task == Compression::compress) target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
        else if (task == Compression::decompress)
        {
            target_stream.open(target_path, std::ios::binary | std::ios::out);  // making sure target file exists
            target_stream.close();
            target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
        }
        assert( archive_stream.is_open() );
        assert( target_stream.is_open() );


        bool* task_finished_arr = new bool[block_count];
        bool* task_started_arr  = new bool[block_count];
        for (uint32_t i=0; i < block_count; ++i) {
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
            if (task == Compression::compress) {
                comp_v[i]->load_part(target_stream, original_size, i, block_size);
                comp_v[i]->part_id = i;
                if (compressed_size != nullptr) *compressed_size = 0;
            }
            else if (task == Compression::decompress) {
                archive_stream.read((char*)&comp_v[i]->part_id, sizeof(comp_v[i]->part_id));
                archive_stream.read((char*)&comp_v[i]->size, sizeof(comp_v[i]->size));
                comp_v[i]->load_text(archive_stream, comp_v[i]->size);
            }
            workers.emplace_back(&processing_worker, task, comp_v[i], flags, std::ref(aborting_var),
                                 &task_finished_arr[i], progress_ptr);
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

        bool successful = false;

        if (task == Compression::compress)
            scribe = std::thread( &processing_scribe, task, std::ref(archive_stream), std::ref(comp_v),
                                  task_finished_arr, block_count, compressed_size,
                                  std::ref(checksum), std::ref(checksum_done), original_size, std::ref(aborting_var), &successful );
        else if (task == Compression::decompress)
            scribe = std::thread( &processing_scribe, task, std::ref(target_stream), std::ref(comp_v), task_finished_arr,
                                  block_count, compressed_size, std::ref(checksum), std::ref(checksum_done),
                                  original_size, std::ref(aborting_var), &successful );

        while (lowest_free_work_ind != block_count and !aborting_var) {

            for (uint32_t i=0; i < workers.size() and !aborting_var; ++i) {
                if (workers[i].joinable()) {
                    if (workers[i].joinable()) workers[i].join();
                    if (aborting_var) break;

                    if (lowest_free_work_ind != block_count) {
                        if (task == Compression::compress) {
                            comp_v[lowest_free_work_ind]->load_part(target_stream, original_size, lowest_free_work_ind, block_size);
                            comp_v[lowest_free_work_ind]->part_id = lowest_free_work_ind;
                        }
                        else if (task == Compression::decompress) {
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
            for (auto& th: workers) if (th.joinable()) th.join();
            if (scribe.joinable()) scribe.join();
            delete[] task_finished_arr;
            delete[] task_started_arr;
            for (auto & comp : comp_v) delete comp;

            return false;
        }

        if (task == Compression::compress) {
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
            checksum_done = true;
        }
        else if (task == Compression::decompress)
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
            checksum_done = true;
        }

        for (auto& th : workers) if (th.joinable()) th.join();
        if (scribe.joinable()) scribe.join();

        delete[] task_finished_arr;
        delete[] task_started_arr;
        for (auto & comp : comp_v) delete comp;

        if (aborting_var) return false;
        return successful;
    }

}
#endif // MULTITHREADING_H
