#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H
// Using any code from this file for anything serious is probably a bad idea.

#include <string>
#include <random>


namespace crypto {

    namespace HMAC {

        // HMAC-SHA256
        std::string SHA256(uint8_t message[], uint64_t message_size,
                           uint8_t key[], uint32_t key_size, bool &aborting_var);
    }

    namespace PBKDF2 {
        static const uint32_t saltSize = 16;
        enum class iteration_count { debug=1000, low=160000, medium=320000, high=720000 };

        namespace {
            std::string HMAC_SHA256_get_block(std::string &pw, uint8_t *salt, uint32_t salt_size,
                                              uint32_t iteration_count, int32_t current_block, bool &aborting_var);
        }
        std::string HMAC_SHA256(std::string &pw, uint8_t salt[], uint32_t salt_size,
                                uint32_t iteration_count, uint32_t dkLen, bool &aborting_var);

    }

    void fill_with_random_data(uint8_t arr[], int64_t arr_size, std::mt19937& gen, int64_t start=0, int64_t stop=-1);

    namespace AES128 // works in CTR mode
    {
        const uint32_t key_size = 16;

        void generate_metadata(uint8_t pwkey[], uint64_t pwkey_size, uint8_t salt[], uint32_t salt_size,
                               uint64_t PBKDF2_iterations, uint8_t random_key[], uint32_t random_key_size,
                               uint8_t*& output, uint32_t& output_size, std::mt19937& gen, bool& aborting_var);

        bool verify_password_key(uint8_t *pwkey, uint32_t pwkey_size, uint8_t *metadata,
                                 uint32_t metadata_size, bool& aborting_var);

        void encrypt(uint8_t*& plaintext, uint64_t& plaintext_size, uint8_t key[], uint32_t key_size,
                     uint8_t iv_arr[], uint32_t iv_size);

        void decrypt(uint8_t*& ciphertext, uint64_t& ciphertext_size, uint8_t key[], uint32_t key_size );
    }
}


#endif //CRYPTOGRAPHY_H
