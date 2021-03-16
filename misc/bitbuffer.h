#ifndef BITBUFFER_H
#define BITBUFFER_H

#include <string>


class TextWriteBitbuffer
{
public:
    uint8_t buffer[8*1024]{};             // 8 KB of buffer
    uint64_t bi{};                        // buffer index
    uint8_t bitcounter{};                 // counts how many bits were added to buffer[bi]
    uint64_t bits_written;
    std::string* text;

    explicit TextWriteBitbuffer(std::string &output_string);
    ~TextWriteBitbuffer();
    void clear();                       // empty the whole buffer
    void flush();                       // send buffer's values to the file
    void add_bit_1();
    void add_bit_0();
    uint64_t get_output_size() const;
};


class TextReadBitbuffer
{
public:
    uint8_t buffer[8*1024]{};           // 8 KB of buffer
    uint64_t byte_index{};              // byte index
    uint8_t bitcounter{};               // counts how many bits were added to buffer[bi]
    uint64_t data_left_b;               // data left (in bits)
    uint16_t bits_in_last_byte;
    uint16_t meaningful_bits;
    bool output_bit;
    uint8_t* text;

    TextReadBitbuffer(uint8_t compressed_text[], uint64_t compressed_size, uint64_t starting_position = 4+4+256*4);
    bool getbit();
};

#endif
