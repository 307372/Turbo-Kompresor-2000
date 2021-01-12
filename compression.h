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
    enum
    {
        compress, decompress
    };
    bool* aborting_var;
    uint8_t* text;
    uint32_t size;
    uint32_t part_id=0;

    Compression( bool& aborting_variable );
    ~Compression();

    void load_text( std::fstream &input, uint64_t text_size );
    void load_part( std::fstream &input, uint64_t text_size, uint32_t part_num, uint32_t block_size );
    void save_text( std::fstream &output );

    void BWT_make();
    void BWT_reverse();

    void MTF_make();
    void MTF_reverse();

    void RLE_make();
    void RLE_reverse();

    void RLE_makeV2();
    void RLE_reverseV2();

    void AC_make();
    void AC_reverse();

    void AC2_make();
    void AC2_reverse();

    void AC2ef_make();
    void AC2ef_reverse();

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
    uint16_t bits_in_last_byte;
    uint16_t meaningful_bits;
    bool output_bit;
    uint8_t* text;
    //std::fstream* os;
    Text_read_bitbuffer(uint8_t compressed_text[], uint64_t compressed_size, uint64_t starting_position = 4+4+256*4);
    bool getbit();
};



#endif //COMPRESSION_DEV_COMPRESSION_H
