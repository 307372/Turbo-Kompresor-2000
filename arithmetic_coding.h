#ifndef ARITHMETIC_CODING_H
#define ARITHMETIC_CODING_H
#include <string>
#include <vector>
#include <limits>
#include <climits>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <map>
#include <string>
#include <iostream>
#include <cmath>
#include <fstream>
#include <bitset>
#include "statistical_tools.h"
#include "project_exceptions.h"

class arithmetic_coding
{
public:
    uint16_t precision;
    uint64_t whole;
    long double wholed;
    uint64_t half;
    uint64_t quarter;
    arithmetic_coding();

    uint64_t encode_file_to_fstream( std::fstream &dst_stream, std::string path_to_target_file, size_t buffer_size );
    void decode_to_file_from_fstream(std::fstream &source, std::string path_for_output, uint64_t compressed_file_size, uint64_t uncompressed_file_size );

};

class output_bitbuffer
{
public:
    uint8_t buffer[8*1024]{};             // 8 KB of buffer
    uint64_t bi{};                        // buffer index
    uint8_t bitcounter{};                 // counts how many bits were added to buffer[bi]
    uint64_t sum_of_outputted_bits;
    std::fstream* os;
    output_bitbuffer(std::fstream &os);
    ~output_bitbuffer();
    void clear();                       // empty the whole buffer
    void flush();                       // send buffer's values to the file
    void add_bit_1();
    void add_bit_0();
    uint64_t get_output_size() const;
};

class input_bitbuffer
{
public:
    uint8_t buffer[8*1024]{};           // 8 KB of buffer
    uint64_t bi{};                      // buffer index
    uint8_t bitcounter{};               // counts how many bits were added to buffer[bi]
    uint64_t data_left_b;               // data left (in bits)
    uint16_t buffer_size;
    uint16_t bits_left_in_buffer;
    uint16_t meaningful_bits;
    bool output_bit;
    std::fstream* os;
    input_bitbuffer(std::fstream &os, uint64_t compressed_size);
    void fill_buffer();
    bool getbit();
};


#endif // ARITHMETIC_CODING_H
