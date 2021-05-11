#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <fstream>
#include <random>
#include <iostream>

struct AES128_metadata {                                                         // 88 bytes in total
    std::basic_string<uint8_t> salt;            // 16 bytes
    uint64_t iterations = 0;                                                              //  8 bytes
    std::basic_string<uint8_t> IV_rkey;         // 16 bytes
    std::basic_string<uint8_t> encrypted_rkey;  // 16 bytes
    std::basic_string<uint8_t> HMAC;            // 32 bytes

private:
    bool loaded = false;

public:
    void load(std::fstream src, int64_t start)
    {
        if (!src.is_open()) {
            throw;
        }
        int64_t initial_pos = src.tellg();
        src.seekg(start);

        std::basic_string<uint8_t> buffer(88, 0);
        src.read((char *) buffer.c_str(), buffer.length());

        src.seekg(initial_pos);

        salt = std::basic_string<uint8_t>(buffer, 0, 16);
        iterations = reinterpret_cast<uint64_t>(buffer.c_str() + 16);
        IV_rkey = std::basic_string<uint8_t>(buffer, 24, 16);
        encrypted_rkey = std::basic_string<uint8_t>(buffer, 40, 16);
        HMAC = std::basic_string<uint8_t>(buffer, 56, 16);

        loaded = true;
    }

    bool is_loaded() {
        return loaded;
    }

    void show() {
        if (loaded) {
            std::cout << "=== Metadata ===\n" << "Salt:\t\t";
            for (uint16_t symbol : salt) {
                std::cout << std::hex << symbol;
            }
            std::cout << "\nIterations:\t" << iterations << '\n';
            std::cout << "IV_rkey:\t";
            for (uint16_t symbol : IV_rkey) {
                std::cout << std::hex << symbol;
            }

            std::cout << "IV_rkey:\t";
            for (uint16_t symbol : IV_rkey) {
                std::cout << std::hex << symbol;
            }

            std::cout << "encrypted_rkey:\t";
            for (uint16_t symbol : encrypted_rkey) {
                std::cout << std::hex << symbol;
            }

            std::cout << "HMAC:\t\t";
            for (uint16_t symbol : HMAC) {
                std::cout << std::hex << symbol;
            }
        }
        else {
            std::cout << "Metadata is not here" << std::endl;
        }
    }
};

class Compression {
private:
    static const uint32_t proper_metadata_size = 88;   // size of metadata should be 88 bytes

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

    void BWT_make();    // Burrows-Wheeler transform (DC3)
    void BWT_reverse();

    void BWT_make2();   // Burrows-Wheeler transform (divsufsort)
    void BWT_reverse2();

    void MTF_make();    // move-to-front
    void MTF_reverse();

    void RLE_make();    // run-length encoding (interlaced)
    void RLE_reverse();

    void RLE_makeV2();  // run-length encoding (separated)
    void RLE_reverseV2();

    void AC_make();     // arithmetic coding (memoryless)
    void AC_reverse();

    void AC2_make();    // arithmetic coding (first-order Markov model)
    void AC2_reverse();

    void AES128_make(uint8_t key[], uint32_t key_size, uint8_t iv[], uint32_t iv_size,
                     uint8_t metadata[]= nullptr, uint8_t metadata_size=0);
    void AES128_reverse(uint8_t key[], uint32_t key_size);

    bool AES128_verify_password_str(std::string& pw, uint8_t *metadata, uint32_t metadata_size);

    void AES128_extract_metadata(uint8_t*& metadata, uint32_t& metadata_size);

};

#endif //COMPRESSION_DEV_COMPRESSION_H
