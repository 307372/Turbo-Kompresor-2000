//
// Created by pc on 03.11.2020.
//

#include "integrity_validation.h"
#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <filesystem>
#include <climits>
#include <bitset>
#include <bit>
#include <iomanip>
#include <map>

integrity_validation::integrity_validation()= default;

std::string integrity_validation::get_SHA1( const std::string& path_to_file, bool& aborting_var )
{
    // implemented using pseudocode from: https://en.wikipedia.org/wiki/SHA-1#SHA-1_pseudocode

    if (!aborting_var) {
        uint64_t file_size = std::filesystem::file_size( path_to_file );


        uint32_t h0 = 1732584193;
        uint32_t h1 = 4023233417;
        uint32_t h2 = 2562383102;
        uint32_t h3 = 271733878;
        uint32_t h4 = 3285377520;

        std::vector<uint8_t> message;
        message.reserve(file_size);

        std::ifstream target_file;
        target_file.open( path_to_file );
        assert( target_file.is_open() );

        uint8_t ch;
        while (target_file >> std::noskipws >> ch) {
            message.push_back(ch);
        }
        message.push_back(0x80);

        while (message.size() % 64 != 56)
            message.push_back(0);

        for (int i = 7; i >= 0; i--) {
            message.push_back((file_size*8 >> i * 8));
        }

        uint32_t a = 0;
        uint32_t b = 0;
        uint32_t c = 0;
        uint32_t d = 0;
        uint32_t e = 0;

        uint32_t f = 0;
        uint32_t k = 0;

        std::vector<uint32_t> chunk;
        chunk.reserve(80);

        for (uint64_t chunk_number = 0; chunk_number < message.size()/64 and !aborting_var; chunk_number++)  //dividing into 512 bit chunks
        {
            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                chunk.push_back(0);
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {

                    word = (word << 8);
                    word += message[ chunk_number*64 + i*4 + j ];
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
                //std::cout << "c = " << c << std::endl;
                b = a;
                a = temp;

            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }

        if (aborting_var) return "";

        std::stringstream stream;
        stream << std::hex << std::setw(8) << std::setfill('0') << h0 <<std::setw(8) << std::setfill('0') << h1 << std::setw(8) << std::setfill('0') << h2 << std::setw(8) << std::setfill('0') << h3 << std::setw(8) << std::setfill('0') << h4;
        std::string sha1hex = stream.str();
        this->SHA1 = sha1hex;
        //std::cout << sha1hex << std::endl;
        return sha1hex;
    }
    return "";
}
