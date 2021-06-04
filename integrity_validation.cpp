#include "integrity_validation.h"

#include <fstream>
#include <cassert>
#include <vector>
#include <filesystem>
#include <bit>
#include <cmath>

IntegrityValidation::IntegrityValidation()
: SHA1_num(nullptr), SHA256_num(nullptr), CRC32_num(nullptr) {
    generate_CRC32_lookup_table();
}

IntegrityValidation::~IntegrityValidation() {
    delete[] SHA1_num;
    delete[] SHA256_num;
    delete[] CRC32_num;
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
        uint32_t chunk[80] = {};

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

                        for (uint32_t i = 0; i < 64; ++i) {
                            leftover_buffer[i] = 0;
                        }

                        for (int i = 0; i<8; ++i) {
                            leftover_buffer[56 + 7 - i] = (byte_counter * 8 >> i * 8) & 0xFF;
                        }

                    } else for (int i = 0; i<8 ; ++i) buffer[56 + 7 - i] = (byte_counter * 8 >> i * 8) & 0xFF;
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


std::string IntegrityValidation::get_SHA256_from_file(const std::string &path_to_file, bool &aborting_var) {
    // based on: https://qvault.io/cryptography/how-sha-2-works-step-by-step-sha-256/

    if (!aborting_var) {
        uint32_t h0 = 0x6A09E667;
        uint32_t h1 = 0xBB67AE85;
        uint32_t h2 = 0x3C6EF372;
        uint32_t h3 = 0xA54FF53A;
        uint32_t h4 = 0x510E527F;
        uint32_t h5 = 0x9B05688C;
        uint32_t h6 = 0x1F83d9AB;
        uint32_t h7 = 0x5BE0CD19;

        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
        uint32_t e;
        uint32_t f;
        uint32_t g;
        uint32_t h;

        uint32_t round_constants[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
                                        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                                        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
                                        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                                        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                                        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
                                        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

        std::ifstream target_file( path_to_file );
        assert( target_file.is_open() );


        uint32_t buffer_size = 64;
        uint8_t buffer[buffer_size];
        uint8_t leftover_buffer[buffer_size];

        uint32_t chunks_size = 64;
        uint32_t chunks[chunks_size];
        for (uint32_t i=0; i < chunks_size; ++i) chunks[i] = 0;

        uint64_t byte_counter = 0;



        bool leftover = false;

        while ( target_file.good() or leftover )
        {
            // loading data
            if (!leftover) {
                target_file.read((char *) &buffer, sizeof(buffer));
                byte_counter += target_file.gcount();

                if (target_file.gcount() < buffer_size) {
                    uint64_t read_counter = target_file.gcount();
                    buffer[read_counter] = 0x80;
                    read_counter++;
                    for (uint32_t i = read_counter; i < buffer_size; ++i) buffer[i] = 0;

                    if (buffer_size - read_counter < 8) {
                        leftover = true;
                        for (uint32_t i = 0; i < buffer_size; ++i) leftover_buffer[i] = 0;
                        for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                    } else for (int i = 0; i<8 ; ++i) buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                }
            }
            else {
                leftover = false;
                for (uint32_t i=0; i < buffer_size; ++i) buffer[i] = leftover_buffer[i];
            }

            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {
                    word = (word << 8);
                    word += buffer[i*4 + j];
                }
                chunks[i] = word;
            }

            uint32_t S0, S1;

            for (uint32_t i=16; i < chunks_size; i++)
            {
                S0 = std::rotr(chunks[i-15], 7) ^ std::rotr(chunks[i-15], 18) ^ (chunks[i-15] >> 3);
                S1 = std::rotr(chunks[i-2], 17) ^ std::rotr(chunks[i-2], 19)  ^ (chunks[i-2] >> 10);
                chunks[i] = chunks[i - 16] + S0 + chunks[i - 7] + S1;
            }

            a = h0;
            b = h1;
            c = h2;
            d = h3;
            e = h4;
            f = h5;
            g = h6;
            h = h7;


            for ( uint32_t i = 0; i < buffer_size; i++)
            {
                S1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + round_constants[i] + chunks[i];
                S0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                h = g;
                g = f;
                f = e;
                e = d+temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
            h5 += f;
            h6 += g;
            h7 += h;
        }
        target_file.close();
        if (aborting_var) return "";

        std::stringstream stream;
        stream << std::hex;
        stream << std::setw(8) << std::setfill('0') << h0;
        stream << std::setw(8) << std::setfill('0') << h1;
        stream << std::setw(8) << std::setfill('0') << h2;
        stream << std::setw(8) << std::setfill('0') << h3;
        stream << std::setw(8) << std::setfill('0') << h4;
        stream << std::setw(8) << std::setfill('0') << h5;
        stream << std::setw(8) << std::setfill('0') << h6;
        stream << std::setw(8) << std::setfill('0') << h7;

        std::string sha256hex = stream.str();
        this->SHA256 = sha256hex;

        delete[] SHA256_num;
        SHA256_num = new uint8_t [8*4]();
        uint32_t copied_ctr = 0;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h0 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h1 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h2 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h3 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h4 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h5 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h6 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h7 >> (24 - i * 8)) & 0xFF;

        return sha256hex;
    }
    return "";
}


std::string IntegrityValidation::get_SHA256_from_stream(std::fstream &target_file, bool &aborting_var) {
    // based on: https://qvault.io/cryptography/how-sha-2-works-step-by-step-sha-256/

    if (!aborting_var) {
        // saving current location in the file, so we could return here after this algorithm
        std::streamoff backup_pos = target_file.tellg();
        target_file.seekg(0);

        uint32_t h0 = 0x6A09E667;
        uint32_t h1 = 0xBB67AE85;
        uint32_t h2 = 0x3C6EF372;
        uint32_t h3 = 0xA54FF53A;
        uint32_t h4 = 0x510E527F;
        uint32_t h5 = 0x9B05688C;
        uint32_t h6 = 0x1F83d9AB;
        uint32_t h7 = 0x5BE0CD19;

        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
        uint32_t e;
        uint32_t f;
        uint32_t g;
        uint32_t h;

        uint32_t round_constants[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
                                        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                                        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
                                        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                                        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                                        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
                                        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

        assert( target_file.is_open() );


        uint32_t buffer_size = 64;
        uint8_t buffer[buffer_size];
        uint8_t leftover_buffer[buffer_size];

        uint32_t chunks_size = 64;
        uint32_t chunks[chunks_size];
        for (uint32_t i=0; i < chunks_size; ++i) chunks[i] = 0;

        uint64_t byte_counter = 0;



        bool leftover = false;

        while ( target_file.good() or leftover )
        {
            // loading data
            if (!leftover) {
                target_file.read((char *) &buffer, sizeof(buffer));
                byte_counter += target_file.gcount();

                if (target_file.gcount() < buffer_size) {
                    uint64_t read_counter = target_file.gcount();
                    buffer[read_counter] = 0x80;
                    read_counter++;
                    for (uint32_t i = read_counter; i < buffer_size; ++i) buffer[i] = 0;

                    if (buffer_size - read_counter < 8) {
                        leftover = true;
                        for (uint32_t i = 0; i < buffer_size; ++i) leftover_buffer[i] = 0;
                        for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                    } else for (int i = 0; i<8 ; ++i) buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                }
            }
            else {
                leftover = false;
                for (uint32_t i=0; i < buffer_size; ++i) buffer[i] = leftover_buffer[i];
            }

            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {
                    word = (word << 8);
                    word += buffer[i*4 + j];
                }
                chunks[i] = word;
            }

            uint32_t S0, S1;

            for (uint32_t i=16; i < chunks_size; i++)
            {
                S0 = std::rotr(chunks[i-15], 7) ^ std::rotr(chunks[i-15], 18) ^ (chunks[i-15] >> 3);
                S1 = std::rotr(chunks[i-2], 17) ^ std::rotr(chunks[i-2], 19)  ^ (chunks[i-2] >> 10);
                chunks[i] = chunks[i - 16] + S0 + chunks[i - 7] + S1;
            }

            a = h0;
            b = h1;
            c = h2;
            d = h3;
            e = h4;
            f = h5;
            g = h6;
            h = h7;


            for ( uint32_t i = 0; i < buffer_size; i++)
            {
                S1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + round_constants[i] + chunks[i];
                S0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                h = g;
                g = f;
                f = e;
                e = d+temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
            h5 += f;
            h6 += g;
            h7 += h;
        }

        target_file.seekg(backup_pos);

        if (aborting_var) return "";



        std::stringstream stream;
        stream << std::hex;
        stream << std::setw(8) << std::setfill('0') << h0;
        stream << std::setw(8) << std::setfill('0') << h1;
        stream << std::setw(8) << std::setfill('0') << h2;
        stream << std::setw(8) << std::setfill('0') << h3;
        stream << std::setw(8) << std::setfill('0') << h4;
        stream << std::setw(8) << std::setfill('0') << h5;
        stream << std::setw(8) << std::setfill('0') << h6;
        stream << std::setw(8) << std::setfill('0') << h7;

        std::string sha256hex = stream.str();
        this->SHA256 = sha256hex;

        delete[] SHA256_num;
        SHA256_num = new uint8_t [8*4]();
        uint32_t copied_ctr = 0;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h0 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h1 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h2 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h3 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h4 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h5 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h6 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h7 >> (24 - i * 8)) & 0xFF;

        return sha256hex;
    }
    return "";
}


std::string IntegrityValidation::get_SHA256_from_text( uint8_t text[], uint64_t text_size, bool& aborting_var ) {
    // based on: https://qvault.io/cryptography/how-sha-2-works-step-by-step-sha-256/

    if (!aborting_var) {
        assert( text != nullptr and text_size != 0 );

        uint32_t h0 = 0x6A09E667;
        uint32_t h1 = 0xBB67AE85;
        uint32_t h2 = 0x3C6EF372;
        uint32_t h3 = 0xA54FF53A;
        uint32_t h4 = 0x510E527F;
        uint32_t h5 = 0x9B05688C;
        uint32_t h6 = 0x1F83d9AB;
        uint32_t h7 = 0x5BE0CD19;

        uint32_t a;
        uint32_t b;
        uint32_t c;
        uint32_t d;
        uint32_t e;
        uint32_t f;
        uint32_t g;
        uint32_t h;

        uint32_t round_constants[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
                                        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
                                        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
                                        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
                                        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
                                        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
                                        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};


        uint32_t buffer_size = 64;
        uint8_t buffer[buffer_size];
        uint8_t leftover_buffer[buffer_size];

        uint32_t chunks_size = 64;
        uint32_t chunks[chunks_size];
        for (uint32_t i=0; i < chunks_size; ++i) chunks[i] = 0;

        uint64_t byte_counter = 0;

        bool leftover = false;

        uint32_t blocks = ceil((double)text_size / (double)buffer_size);

        for (uint32_t current_block = 0; current_block < blocks; ++current_block)
        {
            // loading data
            if (!leftover) {
                if ((current_block+1) * buffer_size < text_size) {
                    for (uint32_t i=(current_block * buffer_size); i < ((current_block+1) * buffer_size); ++i)
                        buffer[i - current_block * buffer_size] = text[i];
                    byte_counter += buffer_size;
                }

                else if ((current_block+1) * buffer_size != text_size)
                {
                    for (uint32_t i=current_block * buffer_size; i < text_size; ++i)
                        buffer[i - current_block * buffer_size] = text[i];
                    uint64_t read_counter = text_size - current_block * buffer_size;
                    byte_counter += read_counter;

                    buffer[read_counter] = 0x80;
                    read_counter++;
                    for (uint32_t i = read_counter; i < buffer_size; ++i) buffer[i] = 0;

                    if (buffer_size - read_counter < 8) {
                        blocks++;
                        leftover = true;
                        for (uint32_t i = 0; i < buffer_size; ++i) leftover_buffer[i] = 0;
                        for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                    } else for (int i = 0; i<8; ++i) buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;
                }
                else {  // (current_block+1) * buffer_size == text_size
                    for (uint32_t i=current_block * buffer_size; i < text_size; ++i)
                        buffer[i - current_block * buffer_size] = text[i];
                    uint64_t read_counter = text_size - current_block * buffer_size;
                    byte_counter += read_counter;
                    blocks++;

                    leftover = true;
                    for (uint32_t i = 0; i < buffer_size; ++i) leftover_buffer[i] = 0;
                    leftover_buffer[0] = 0x80;
                    for (int i = 0; i<8; ++i) leftover_buffer[56 + 7-i] = (byte_counter * 8 >> i * 8) & 0xFF;//*/
                }
            }
            else {
                leftover = false;
                for (uint32_t i=0; i < buffer_size; ++i) buffer[i] = leftover_buffer[i];
            }

            for (uint8_t i = 0; i < 16; i++) // making 16 32-bit words from 64 8-bit words
            {
                uint64_t word = 0;
                for (uint8_t j = 0; j < 4; j++) // making single 32-bit word
                {
                    word = (word << 8);
                    word += buffer[i*4 + j];
                }
                chunks[i] = word;
            }

            uint32_t S0, S1;

            for (uint32_t i=16; i < chunks_size; i++)
            {
                S0 = std::rotr(chunks[i-15], 7) ^ std::rotr(chunks[i-15], 18) ^ (chunks[i-15] >> 3);
                S1 = std::rotr(chunks[i-2], 17) ^ std::rotr(chunks[i-2], 19)  ^ (chunks[i-2] >> 10);
                chunks[i] = chunks[i - 16] + S0 + chunks[i - 7] + S1;
            }


            a = h0;
            b = h1;
            c = h2;
            d = h3;
            e = h4;
            f = h5;
            g = h6;
            h = h7;


            for ( uint32_t i = 0; i < buffer_size; i++)
            {
                S1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = h + S1 + ch + round_constants[i] + chunks[i];
                S0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;

                h = g;
                g = f;
                f = e;
                e = d+temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
            h5 += f;
            h6 += g;
            h7 += h;
        }
        if (aborting_var) return "";


        std::stringstream stream;
        stream << std::hex;
        stream << std::setw(8) << std::setfill('0') << h0;
        stream << std::setw(8) << std::setfill('0') << h1;
        stream << std::setw(8) << std::setfill('0') << h2;
        stream << std::setw(8) << std::setfill('0') << h3;
        stream << std::setw(8) << std::setfill('0') << h4;
        stream << std::setw(8) << std::setfill('0') << h5;
        stream << std::setw(8) << std::setfill('0') << h6;
        stream << std::setw(8) << std::setfill('0') << h7;

        std::string sha256hex = stream.str();
        this->SHA256 = sha256hex;

        delete[] SHA256_num;
        SHA256_num = new uint8_t [8*4]();
        uint32_t copied_ctr = 0;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h0 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h1 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h2 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h3 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h4 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h5 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h6 >> (24 - i * 8)) & 0xFF;
        for (uint32_t i=0; i < 4; ++i) SHA256_num[copied_ctr++] = (h7 >> (24 - i * 8)) & 0xFF;


        return sha256hex;
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
    for (uint64_t i=0; i < text_size; ++i and !aborting_var)
        crc32 = (crc32 >> 8) xor CRC32_lookup_table[(crc32 xor (uint32_t)text[i]) & 0xFF];

    if (!aborting_var) {
        std::stringstream stream;
        stream << "0x" << std::hex << std::setw(8) << std::setfill('0') << ~crc32;
        std::string crc32_str = stream.str();
        this->CRC32 = crc32_str;
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
        this->CRC32 = crc32_str;
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
        this->CRC32 = crc32_str;
        return crc32_str;
    }
    else return "";
}


