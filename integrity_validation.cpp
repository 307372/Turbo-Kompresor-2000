#include "integrity_validation.h"

#include <fstream>
#include <cassert>
#include <vector>
#include <filesystem>
#include <climits>
#include <bitset>
#include <bit>


IntegrityValidation::IntegrityValidation() {
    generate_CRC32_lookup_table();
}


std::string IntegrityValidation::get_SHA1_from_file(const std::string &path_to_file, bool &aborting_var) {
    // implemented using pseudocode from: https://en.wikipedia.org/wiki/SHA-1#SHA-1_pseudocode

    if (!aborting_var) {
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        std::ifstream target_file( path_to_file );
        assert( target_file.is_open() );


        uint32_t a = 0;
        uint32_t b = 0;
        uint32_t c = 0;
        uint32_t d = 0;
        uint32_t e = 0;

        uint32_t f = 0;
        uint32_t k = 0;

        uint8_t buffer[64];
        uint8_t leftover_buffer[64];
        uint32_t chunk[80];
        for (uint32_t i=0; i < 80; ++i) chunk[i] = 0;

        uint64_t byte_counter = 0;

        bool leftover = false;

        while ( target_file.good() or leftover )
        {
            if (!leftover) {
                target_file.read((char *) &buffer, sizeof(buffer));
                byte_counter += target_file.gcount();

                if (target_file.gcount() < 64) {
                    uint64_t read_counter = target_file.gcount();
                    buffer[read_counter] = 0x80;
                    read_counter++;
                    for (uint32_t i = read_counter; i < 64; ++i) buffer[i] = 0;

                    if (64 - read_counter < 8) {
                        leftover = true;
                        for (uint32_t i = 0; i < 64; ++i) leftover_buffer[i] = 0;
                        for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                    } else for (int i = 0; i<8 ; ++i) buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                }
            }
            else {
                leftover = false;
                for (uint32_t i=0; i < 64; ++i) buffer[i] = leftover_buffer[i];
            }

            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {
                    word = (word << 8);
                    word += buffer[ i*4 + j ];
                }
                chunk[i] = word;
            }
            for (uint8_t id=16; id < 80; id++)
            {
                chunk[id] =  std::rotl(chunk[id-3] ^ chunk[id-8] ^ chunk[id-14] ^ chunk[id-16], 1);
            }


            a = h0;
            b = h1;
            c = h2;
            d = h3;
            e = h4;


            uint32_t temp = 0;
            for ( uint8_t i = 0; i < 80; i++)
            {
                if ( (0 <= i) and (i <= 19) )
                {
                    f = ( b & c ) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if ((20 <= i) and (i <= 39))
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if ((40 <= i) and (i <= 59)) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else if ((60 <= i) and (i <= 79)) {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                temp = std::rotl(a, 5) + f + e + k + chunk[i];
                e = d;
                d = c;
                c = std::rotl(b, 30);
                b = a;
                a = temp;

            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }
        target_file.close();
        if (aborting_var) return "";

        std::stringstream stream;
        stream << std::hex << std::setw(8) << std::setfill('0') << h0 <<std::setw(8) << std::setfill('0') << h1 << std::setw(8) << std::setfill('0') << h2 << std::setw(8) << std::setfill('0') << h3 << std::setw(8) << std::setfill('0') << h4;
        std::string sha1hex = stream.str();
        this->SHA1 = sha1hex;
        return sha1hex;
    }
    return "";
}


std::string IntegrityValidation::get_SHA1_from_stream(std::fstream &target_file, uint64_t file_size, bool &aborting_var) {
    // implemented using pseudocode from: https://en.wikipedia.org/wiki/SHA-1#SHA-1_pseudocode

    if (!aborting_var) {

        uint64_t backup_pos = target_file.tellg();

        target_file.seekg(0);

        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;

        assert( target_file.is_open() );


        uint32_t a = 0;
        uint32_t b = 0;
        uint32_t c = 0;
        uint32_t d = 0;
        uint32_t e = 0;

        uint32_t f = 0;
        uint32_t k = 0;

        uint8_t buffer[64];

        std::vector<uint32_t> chunk(80, 0);
        chunk.reserve(80);

        uint64_t byte_counter = 0;

        bool leftover = false;
        uint8_t leftover_buffer[64];

        while ( target_file.good() or leftover )
        {
            if (!leftover) {
                target_file.read((char *) &buffer, sizeof(buffer));
                byte_counter += target_file.gcount();

                if (target_file.gcount() < 64) {
                    uint64_t read_counter = target_file.gcount();
                    buffer[read_counter] = 0x80;
                    read_counter++;
                    for (uint32_t i = read_counter; i < 64; ++i) buffer[i] = 0;

                    if (64 - read_counter < 8) {
                        leftover = true;
                        for (uint32_t i = 0; i < 64; ++i) leftover_buffer[i] = 0;
                        for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                    } else for (int i = 0; i<8 ; ++i) buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                }
            }
            else {
                leftover = false;
                for (uint32_t i=0; i < 64; ++i) buffer[i] = leftover_buffer[i];
            }

            chunk = std::vector<uint32_t>();
            chunk.reserve(80);

            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                chunk.push_back(0);
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {
                    word = (word << 8);
                    word += buffer[ i*4 + j ];
                }
                chunk[i] = word;
            }
            for (uint8_t id =16; id < 80; id++)
            {
                chunk.push_back(0);
                chunk[id] =  std::rotl(chunk[id-3] ^ chunk[id-8] ^ chunk[id-14] ^ chunk[id-16], 1);
            }


            a = h0;
            b = h1;
            c = h2;
            d = h3;
            e = h4;


            uint32_t temp = 0;
            for ( uint8_t i = 0; i < 80; i++)
            {
                if ( (0 <= i) and (i <= 19) )
                {
                    f = ( b & c ) | ((~b) & d);
                    k = 0x5A827999;
                }
                else if ((20 <= i) and (i <= 39))
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if ((40 <= i) and (i <= 59)) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                }
                else if ((60 <= i) and (i <= 79)) {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                temp = std::rotl(a, 5) + f + e + k + chunk[i];
                e = d;
                d = c;
                c = std::rotl(b, 30);
                b = a;
                a = temp;

            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        target_file.seekg(backup_pos);

        if (aborting_var) return "";

        std::stringstream stream;
        stream << std::hex << std::setw(8) << std::setfill('0') << h0 <<std::setw(8) << std::setfill('0') << h1 << std::setw(8) << std::setfill('0') << h2 << std::setw(8) << std::setfill('0') << h3 << std::setw(8) << std::setfill('0') << h4;
        std::string sha1hex = stream.str();
        this->SHA1 = sha1hex;
        return sha1hex;
    }
    return "";
}


void IntegrityValidation::generate_CRC32_lookup_table() {
    //source: https://stackoverflow.com/questions/26049150/calculate-a-32-bit-crc-lookup-table-in-c-c/26051190
    uint64_t poly = 0xEDB88320; // reversed 0x4C11DB7
    uint64_t remainder;
    for (uint16_t b = 0; b < 256; ++b) {
        remainder = b;
        for (uint64_t bit=8; bit > 0; --bit) {
            if (remainder & 1)
                remainder = (remainder >> 1) xor poly;
            else
                remainder >>= 1;
        }
        CRC32_lookup_table[b] = remainder;
    }
}


std::string IntegrityValidation::get_CRC32_from_text(uint8_t *text, uint64_t text_size, bool& aborting_var) {
    // Based on pseudocode from wikipedia:
    // https://en.wikipedia.org/wiki/Cyclic_redundancy_check#CRC-32_algorithm
    uint32_t crc32 = UINT32_MAX;
    for (uint64_t i=0; i < text_size; ++i and !aborting_var) crc32 = (crc32 >> 8) xor CRC32_lookup_table[(crc32 xor (uint32_t)text[i]) & 0xFF];

    if (!aborting_var) {
        std::stringstream stream;
        stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << ~crc32;
        std::string crc32_str = stream.str();
        return crc32_str;
    }
    else return "";
}


std::string IntegrityValidation::get_CRC32_from_file( std::string path, bool& aborting_var ) {
    // Based on pseudocode from wikipedia:
    // https://en.wikipedia.org/wiki/Cyclic_redundancy_check#CRC-32_algorithm

    std::fstream source(path, std::ios::binary | std::ios::in);
    uint8_t buffer[8*1024];


    uint32_t crc32 = UINT32_MAX;
    while (source.good())
    {
        source.read((char*)&buffer, sizeof(buffer));

        for (uint64_t i=0; i < source.gcount(); ++i)
        {
            crc32 = (crc32 >> 8) xor CRC32_lookup_table[(crc32 xor (uint32_t)buffer[i]) & 0xFF];
        }
    }

    if (!aborting_var) {
        std::stringstream stream;
        stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << ~crc32;
        std::string crc32_str = stream.str();
        return crc32_str;
    }
    else return "";
}


std::string IntegrityValidation::get_CRC32_from_stream(std::fstream &source, bool &aborting_var) {
    // Based on pseudocode from wikipedia:
    // https://en.wikipedia.org/wiki/Cyclic_redundancy_check#CRC-32_algorithm

    assert( source.is_open() );
    uint64_t backup_pos = source.tellg();

    uint8_t buffer[8*1024];

    source.seekg(0);

    uint32_t crc32 = UINT32_MAX;
    while (source.good())
    {
        source.read((char*)&buffer, sizeof(buffer));

        for (uint64_t i=0; i < source.gcount(); ++i)
        {
            crc32 = (crc32 >> 8) xor CRC32_lookup_table[(crc32 xor (uint32_t)buffer[i]) & 0xFF];
        }
    }


    source.seekg(backup_pos);
    if (!aborting_var) {
        std::stringstream stream;
        stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << ~crc32;
        std::string crc32_str = stream.str();
        return crc32_str;
    }
    else return "";
}
