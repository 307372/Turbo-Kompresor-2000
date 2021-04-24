#ifndef CRYPTOGRAPHY_CPP
#define CRYPTOGRAPHY_CPP

#include "cryptography.h"

#include <cmath>
#include <cassert>

#include "integrity_validation.h"

namespace crypto {

    namespace HMAC {

        std::string SHA256(uint8_t message[], uint64_t message_size,
                           uint8_t key[], uint32_t key_size, bool &aborting_var) {
            uint32_t block_size = 64;   // block size of SHA-256
            uint32_t output_size = 32;  // output size of SHA-256
            auto *block_sized_key = new uint8_t[block_size]();
            // initialized with zeros, so if the key is too short, it's padded in advance
            // padding key with zeros is suggested here: https://tools.ietf.org/html/rfc2104

            if (key_size > block_size) {
                // if the key is longer than block size, we'll hash it
                IntegrityValidation val;
                val.get_SHA256_from_text(key, key_size, aborting_var);

                for (uint32_t i = 0; i < output_size; ++i)
                    block_sized_key[i] = val.SHA256_num[i];
            } else {
                // if the key is not longer than block size, we'll just copy it
                for (uint32_t i = 0; i < key_size; ++i)
                    block_sized_key[i] = key[i];
            }

            uint8_t opad = 0x5C;
            uint8_t ipad = 0x36;


            uint32_t con_size = block_size + message_size;    // size of concatenated array
            auto *concatenated = new uint8_t[con_size]();


            uint32_t concatenated_index = 0;

            for (uint32_t i = 0; i < block_size; ++i)
                concatenated[concatenated_index++] = block_sized_key[i] ^ ipad;

            for (uint64_t i = 0; i < message_size; ++i)
                concatenated[concatenated_index++] = message[i];

            IntegrityValidation val;
            val.get_SHA256_from_text(concatenated, concatenated_index, aborting_var);
            delete[] concatenated;

            // next, we'll need to concatenate (block-sized key xor opad) || SHA-256((block-sized key xor ipad) || message)
            // and hash it
            auto *concat2 = new uint8_t[block_size + output_size];
            uint32_t concatenated_index2 = 0;

            for (uint32_t i = 0; i < block_size; ++i)
                concat2[concatenated_index2++] = block_sized_key[i] ^ opad;
            delete[] block_sized_key;

            for (uint32_t i = 0; i < output_size; ++i) {
                concat2[concatenated_index2++] = val.SHA256_num[i];
            }

            assert(concatenated_index2 == block_size + output_size);

            val.get_SHA256_from_text(concat2, concatenated_index2, aborting_var);

            delete[] concat2;

            assert(val.SHA256_num != nullptr);

            return std::string((char *) val.SHA256_num, 32);
        }
    }

    namespace PBKDF2 {
        std::string HMAC_SHA256_get_block(std::string &pw, uint8_t *salt, uint32_t salt_size,
                                          uint32_t iteration_count, int32_t current_block, bool &aborting_var)
// pw - password
        {
            uint32_t sha256_size = 32; // size of the output of SHA-256, and HMAC-SHA-256

            std::string U((char *) salt, salt_size);
            for (uint32_t it = 0; it < 4; ++it)
                U.push_back((char) (((current_block >> (24 - it * 8)) & 0xFF)));

            U = HMAC::SHA256((uint8_t *) U.c_str(), U.length(), (uint8_t *) pw.c_str(), pw.length(), aborting_var);
            std::string xored = U;

            for (uint32_t it = 1; it < iteration_count; ++it) {
                U = HMAC::SHA256((uint8_t *) U.c_str(), U.length(), (uint8_t *) pw.c_str(), pw.length(), aborting_var);

                assert(U.length() == sha256_size);
                assert(xored.length() == sha256_size);

                for (uint32_t j = 0; j < sha256_size; ++j)
                    xored[j] = (char) (xored[j] ^ U[j]);
            }

            return xored;
        }


        std::string HMAC_SHA256(std::string &pw, uint8_t salt[], uint32_t salt_size,
                                uint32_t iteration_count, uint32_t dkLen, bool &aborting_var)
// based on: https://www.ietf.org/rfc/rfc2898.txt
// pw         password

// dkLen      intended length in bytes of the derived
//            key, a positive integer, at most
//            (2^32 - 1) * hLen
        {
            uint32_t hLen = 32;   // size of the output of SHA-256, and HMAC-SHA-256 in bytes
            assert(dkLen < (2 ^ 32 - 1) * hLen);

            // l - number of hLen-sized blocks in the derived key
            uint32_t l = ceil((double) dkLen / hLen);

            std::string output(dkLen, 0x00);
            uint32_t output_i = 0;

            for (int32_t i = 1; i <= l; ++i) {
                std::string T = HMAC_SHA256_get_block(pw, salt, salt_size, iteration_count, i, aborting_var);

                // concatenating Ts we got for every block, by copying them in the output
                if (i != l) {
                    for (char sign : T) {
                        output[output_i++] = sign;
                    }
                } else {  // if current T is the last one, we'll have to make sure we've not copied more chars than dkLen
                    for (uint32_t j = 0; output_i < dkLen; ++j) {
                        output[output_i++] = T[j];
                    }
                }
            }

            return output;
        }
    }
}


#endif // CRYPTOGRAPHY_CPP
