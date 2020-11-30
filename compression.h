//
// Created by pc on 20.11.2020.
//

#ifndef COMPRESSION_DEV_COMPRESSION_H
#define COMPRESSION_DEV_COMPRESSION_H
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <chrono>
#include <list>
#include <climits>
#include <cmath>
#include <divsufsort.h> // external library
#include "statistical_tools.h"
//#include "project_exceptions.h"



class Compression {
public:
    uint8_t* text;
    uint32_t size;

    Compression();
    ~Compression();

    void load_text( std::fstream &input, uint64_t &text_size );
    void save_text( std::fstream &output );

    void BWT_make();
    void BWT_reverse();

    void MTF_make();
    void MTF_reverse();

    void RLE_make();
    void RLE_reverse();

    void AC_make();
    void AC_reverse();

};

class Text_write_bitbuffer
{
public:
    uint8_t buffer[8*1024]{};             // 8 KB of buffer
    uint64_t bi{};                        // buffer index
    uint8_t bitcounter{};                 // counts how many bits were added to buffer[bi]
    uint64_t sum_of_outputted_bits;
    std::string* text;
    explicit Text_write_bitbuffer(std::string &output_string);
    ~Text_write_bitbuffer();
    void clear();                       // empty the whole buffer
    void flush();                       // send buffer's values to the file
    void add_bit_1();
    void add_bit_0();
    uint64_t get_output_size() const;
};

class Text_read_bitbuffer
{
public:
    uint8_t buffer[8*1024]{};           // 8 KB of buffer
    uint64_t byte_index{};              // byte index
    uint8_t bitcounter{};               // counts how many bits were added to buffer[bi]
    uint64_t data_left_b;               // data left (in bits)
    //uint16_t buffer_size;
    uint16_t bits_left_in_buffer;
    uint16_t meaningful_bits;
    bool output_bit;
    uint8_t* text;
    //std::fstream* os;
    Text_read_bitbuffer(uint8_t compressed_text[], uint64_t compressed_size);
    bool getbit();
};


#endif //COMPRESSION_DEV_COMPRESSION_H
