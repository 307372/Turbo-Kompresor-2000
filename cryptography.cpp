#ifndef CRYPTOGRAPHY_CPP
#define CRYPTOGRAPHY_CPP

#include "cryptography.h"

#include <cmath>
#include <cassert>
#include <immintrin.h>

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
        namespace {
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
                    U = HMAC::SHA256((uint8_t *) U.c_str(), U.length(), (uint8_t *) pw.c_str(), pw.length(),
                                     aborting_var);

                    assert(U.length() == sha256_size);
                    assert(xored.length() == sha256_size);

                    for (uint32_t j = 0; j < sha256_size; ++j)
                        xored[j] = (char) (xored[j] ^ U[j]);
                }

                return xored;
            }
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

    namespace AES128 {
        // helper functions
        namespace
        {

            inline void increment_128b(__m128i& val, uint8_t val_arr[])
            {
                for (int32_t i=15; i >= 0; --i)
                {
                    ++val_arr[i];
                    if (val_arr[i] != 0x00)
                        break;
                }
                val = *((__m128i*) val_arr);
            }

            template <int round_counstant>
            __m128i aes128_key_expansion(__m128i subkey) {
                // source: https://stackoverflow.com/questions/32297088/how-to-implement-aes128-encryption-decryption-using-aes-ni-instructions-and-gcc
                __m128i key_assisted = _mm_aeskeygenassist_si128(subkey, round_counstant);
                subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
                subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
                subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
                key_assisted = _mm_shuffle_epi32(key_assisted, _MM_SHUFFLE(3, 3, 3, 3));
                return _mm_xor_si128(subkey, key_assisted);
            }

            void AES128_get_subkeys(uint8_t *key, __m128i *round_keys)
            {
                round_keys[0] = _mm_loadu_si128((__m128i*) key);
                round_keys[1] = aes128_key_expansion<0x01>(round_keys[0]);
                round_keys[2] = aes128_key_expansion<0x02>(round_keys[1]);
                round_keys[3] = aes128_key_expansion<0x04>(round_keys[2]);
                round_keys[4] = aes128_key_expansion<0x08>(round_keys[3]);
                round_keys[5] = aes128_key_expansion<0x10>(round_keys[4]);
                round_keys[6] = aes128_key_expansion<0x20>(round_keys[5]);
                round_keys[7] = aes128_key_expansion<0x40>(round_keys[6]);
                round_keys[8] = aes128_key_expansion<0x80>(round_keys[7]);
                round_keys[9] = aes128_key_expansion<0x1B>(round_keys[8]);
                round_keys[10]= aes128_key_expansion<0x36>(round_keys[9]);
            }
        }

        void generate_metadata(uint8_t pwkey[], uint64_t pwkey_size, uint8_t salt[], uint32_t salt_size,
                               uint64_t PBKDF2_iterations, uint8_t random_key[], uint32_t random_key_size,
                               uint8_t*& output, uint32_t& output_size, std::mt19937& gen, bool& aborting_var)
        // layout of metadata:
        //  0-15 - salt
        // 16-23 - iteration count
        // 24-39 - IV for random key
        // 40-55 - random key encrypted with AES128 using key=PBKDF2(password)
        // 56-87 - HMAC(msg=IV | encrypted random key, key=random key)
        {

            const uint32_t AES128_key_size = 16;

            assert(random_key_size == AES128_key_size);
            assert(salt_size == crypto::PBKDF2::saltSize);

            uint32_t metadata_size = 88;
            auto* metadata = new uint8_t[metadata_size]();


            uint32_t output_i = 0;
            for (uint32_t i=0; i < salt_size; ++i) {                                // copying the salt
                metadata[output_i++] = salt[i];
            }

            for (uint64_t i=0; i < sizeof(PBKDF2_iterations); ++i) {                // copying the number of iterations
                metadata[output_i++] = (uint8_t) (PBKDF2_iterations >> (i * 8u)) & 0xFFu;
            }

            // generating IV for encrypting the random key
            uint32_t iv_size = AES128_key_size;
            auto* iv_random_key = new uint8_t [iv_size];
            crypto::fill_with_random_data(iv_random_key, iv_size, gen);
            // print_arr_8(iv_random_key, iv_size, "IV random key:");

            uint64_t encrypted_random_key_size = random_key_size;                   // will be overwritten during encryption
            auto* encrypted_random_key = new uint8_t [encrypted_random_key_size];
            for (uint32_t i=0; i < encrypted_random_key_size; ++i) {
                encrypted_random_key[i] = random_key[i];
            }
            // print_arr_8(encrypted_random_key, encrypted_random_key_size, "Random key:");


            // replaces encrypted_random_key with concatenation of iv and encrypted random key
            crypto::AES128::encrypt(encrypted_random_key, encrypted_random_key_size,
                                    pwkey, pwkey_size, iv_random_key, iv_size);

            // print_arr_8(encrypted_random_key, encrypted_random_key_size, "Encrypted random key:");
            delete[] iv_random_key;

            for (uint32_t i=0; i < encrypted_random_key_size; ++i) {                    // copying the IV and encrypted random key
                metadata[output_i++] = encrypted_random_key[i];
            }

            std::string HMAC = crypto::HMAC::SHA256(encrypted_random_key, encrypted_random_key_size,
                                                    random_key, random_key_size, aborting_var);

            delete[] encrypted_random_key;

            for (uint32_t i=0; i < HMAC.length(); ++i) {                                // copying the HMAC
                metadata[output_i++] = (uint8_t) HMAC[i];
            }

            assert(output_i == 88);
            std::swap(metadata, output);
            std::swap(metadata_size, output_size);
            delete[] metadata;
        }

        bool verify_password_key(uint8_t *pwkey, uint32_t pwkey_size, uint8_t *metadata,
                                        uint32_t metadata_size, bool& aborting_var) {
            assert(pwkey_size == crypto::AES128::key_size);
            const uint32_t aes128_key_size = crypto::AES128::key_size;

            // we'll need to parse metadata to verify the password

            uint64_t loading_offset = crypto::PBKDF2::saltSize + sizeof(uint64_t);

            uint64_t encrypted_key_size = 16+16;    // 0-15 - IV, 16-31 - ciphertext
            auto* encrypted_key = new uint8_t [encrypted_key_size];
            for (uint32_t i=0; i < encrypted_key_size; ++i) {
                encrypted_key[i] = metadata[loading_offset++];
            }

            crypto::AES128::decrypt(encrypted_key, encrypted_key_size, pwkey, pwkey_size);

            uint32_t hmac_size = 32;    // this is specific to HMAC-SHA256
            std::string generated_hmac = crypto::HMAC::SHA256(metadata + crypto::PBKDF2::saltSize + sizeof(uint64_t),
                                                              hmac_size, encrypted_key, encrypted_key_size, aborting_var);


            auto* hmac = new uint8_t [hmac_size];


            for (uint64_t i=0; i < hmac_size; ++i) {                            // loading HMAC-SHA256
                hmac[i] = metadata[loading_offset++];
            }

            bool correct_password = true;

            for (uint32_t i=0; i < hmac_size; ++i){
                if (hmac[i] != (uint8_t) generated_hmac[i]) {
                    correct_password = false;
                    break;
                }
            }

            delete[] hmac;
            delete[] encrypted_key;
            return correct_password;
        }

        void encrypt(uint8_t*& plaintext, uint64_t& plaintext_size, uint8_t key[], uint32_t key_size,
                     uint8_t iv_char[], uint32_t iv_size)
        // Layout - 1st 16 bytes - IV, next plaintext_size bytes - ciphertext
        {
            assert(key_size == 16);
            assert(iv_size == 16);

            if (plaintext_size != 0) {
                // copying iv_arr, since we'll be modifying it
                auto* iv_arr = new uint8_t [iv_size];
                for (uint32_t i=0; i < iv_size; ++i) {
                    iv_arr[i] = iv_char[i];
                }

                int64_t amount_of_blocks = ceill((long double)plaintext_size / 16.0);    // calculating the amount of blocks of ciphertext
                __m128i iv = *((__m128i*) iv_arr);
                __m128i round_keys[11] = {};
                AES128_get_subkeys(key, round_keys);

                uint64_t output_size = sizeof(iv) + plaintext_size;  // we'll need to save initialization vector with encrypted data
                auto* output = new uint8_t [output_size]();
                _mm_storeu_si128((__m128i*)(output), iv);       // saving initialization vector to the beginning of the output


                __m128i ctr = iv;   // counter
                __m128i ctr_enc;    // encrypted counter
                __m128i block;      // block of plaintext

                for(uint64_t i=0; i < amount_of_blocks-1; ++i)
                {
                    block = _mm_loadu_si128((__m128i*) (plaintext + i * 16));    // loading a block of plaintext

                    // in ctr (counter) mode, we'll encrypt counter and then xor it with plaintext to get ciphertext
                    ctr_enc = _mm_xor_si128(round_keys[0], ctr);    // xoring ctr with key

                    // 10 rounds of SubBytes, ShiftRows, MixColumns, AddRoundKey
                    for (uint32_t j=1; j <= 9; ++j) ctr_enc = _mm_aesenc_si128(ctr_enc, round_keys[j]);
                    ctr_enc = _mm_aesenclast_si128(ctr_enc, round_keys[10]);

                    block = _mm_xor_si128(block, ctr_enc);  // xoring block of data with encrypted counter

                    _mm_storeu_si128((__m128i*)(output+(i+1)*16), block);   // saving encrypted data offset by 1 block (1st one is the iv)

                    increment_128b(ctr, iv_arr);
                }

                // processing last block
                block = _mm_loadu_si128((__m128i*)(plaintext + (amount_of_blocks - 1) * 16));
                ctr_enc = _mm_xor_si128(round_keys[0], ctr);    // xoring ctr with key

                // 10x SubBytes, ShiftRows, MixColumns, AddRoundKey
                for (uint32_t j=1; j <= 9; ++j) ctr_enc = _mm_aesenc_si128(ctr_enc, round_keys[j]);
                ctr_enc = _mm_aesenclast_si128(ctr_enc, round_keys[10]);

                // xoring block of data with our encrypted counter will give us our ciphertext
                block = _mm_xor_si128(block, ctr_enc);

                // calculating how much data should be written
                uint32_t reminder = plaintext_size % 16;
                if (reminder == 0) reminder = 16;

                for (uint32_t i=0; i < reminder; ++i)
                {
                    if (i < 8)
                    {
                        uint8_t val = ((block[0] >>(8*i)) & 0xFF);
                        output[plaintext_size - reminder + i + 16] = val;
                    }
                    else
                    {
                        uint8_t val = ((block[1] >>(8*(i-8))) & 0xFF);
                        output[plaintext_size - reminder + i + 16] = val;
                    }
                }

                _mm_empty();    // cleaning up MMX register

                // swapping output with input
                std::swap(plaintext, output);
                std::swap(plaintext_size, output_size);
                delete[] output;
                delete[] iv_arr;
            }
        }


        void decrypt(uint8_t*& ciphertext, uint64_t& ciphertext_size, uint8_t key[], uint32_t key_size)
        {
            assert(key_size == 16);

            int64_t amount_of_blocks = ceill((long double)ciphertext_size / 16.0);
            if (ciphertext_size != 0) {

                __m128i round_keys[11] = {};
                AES128_get_subkeys(key, round_keys);
                __m128i iv = _mm_loadu_si128((__m128i*) ciphertext);

                uint64_t output_size = ciphertext_size - sizeof(iv);  // we'll won't need IV in our plaintext
                auto* output = new uint8_t [output_size]();
                _mm_storeu_si128((__m128i*)(output), iv);

                __m128i ctr = iv;
                __m128i ctr_enc;
                __m128i block;
                for(uint64_t i=1; i < amount_of_blocks-1; ++i)  // -1 block, because if it's not 128b, then we'll go out of range
                {
                    block = _mm_loadu_si128((__m128i*)(ciphertext + i * 16)); // loading a block of ciphertext
                    // in ctr (counter) mode, we'll encrypt counter and then xor it with plaintext to get ciphertext
                    ctr_enc = _mm_xor_si128(round_keys[0], ctr);    // xoring ctr with key

                    // 10x SubBytes, ShiftRows, MixColumns, AddRoundKey
                    for (uint32_t j=1; j <= 9; ++j) ctr_enc = _mm_aesenc_si128(ctr_enc, round_keys[j]);
                    ctr_enc = _mm_aesenclast_si128(ctr_enc, round_keys[10]);

                    // xoring block of data with our encrypted counter will give us our ciphertext
                    block = _mm_xor_si128(block, ctr_enc);
                    _mm_storeu_si128((__m128i*)(output+(i-1)*16), block);    // saving plaintext, we're skipping 1st block of
                    // ciphertext, as it contained IV
                    increment_128b(ctr, ciphertext);   // first 16 bytes of ciphertext are IV
                }

                // processing last block
                block = _mm_loadu_si128((__m128i*)(ciphertext + (amount_of_blocks - 1) * 16));
                ctr_enc = _mm_xor_si128(round_keys[0], ctr);    // xoring ctr with key

                // 10x SubBytes, ShiftRows, MixColumns, AddRoundKey
                for (uint32_t j=1; j <= 9; ++j) ctr_enc = _mm_aesenc_si128(ctr_enc, round_keys[j]);
                ctr_enc = _mm_aesenclast_si128(ctr_enc, round_keys[10]);    // last round is a bit different

                // xoring block of data with our encrypted counter will give us our ciphertext
                block = _mm_xor_si128(block, ctr_enc);

                // calculating how much data should be written
                uint32_t reminder = ciphertext_size % 16;
                if (reminder == 0) reminder = 16;

                for (uint32_t i=0; i < reminder; ++i)
                {
                    if (i < 8)
                    {
                        uint8_t val = ((block[0] >>(8*i)) & 0xFF);
                        output[ciphertext_size - reminder + i - 16] = val;
                    }
                    else
                    {
                        uint8_t val = ((block[1] >>(8*(i-8))) & 0xFF);
                        output[ciphertext_size - reminder + i - 16] = val;
                    }
                }

                _mm_empty();    // cleaning up MMX register

                // swapping input with output
                std::swap(ciphertext, output);
                std::swap(ciphertext_size, output_size);
                delete[] output;
            }
        }
    }

    void fill_with_random_data(uint8_t arr[], int64_t arr_size, std::mt19937& gen, int64_t start, int64_t stop)
    {
        if (arr_size < 0)
            return;

        if (stop < 0)
            stop = arr_size-1;

        for (int64_t i=start; i <= stop; ++i)
            arr[i] = gen() & 0xFF;
    }
}


#endif // CRYPTOGRAPHY_CPP
