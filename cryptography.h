#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H
// Using any code from this file for anything serious is probably a bad idea.

#include <string>

namespace crypto {

    namespace HMAC {

        // HMAC-SHA256
        std::string SHA256(uint8_t message[], uint64_t message_size,
                           uint8_t key[], uint32_t key_size, bool &aborting_var);
    }

    namespace PBKDF2 {
        std::string HMAC_SHA256_get_block(std::string &pw, uint8_t *salt, uint32_t salt_size,
                                          uint32_t iteration_count, int32_t current_block, bool &aborting_var);

        std::string HMAC_SHA256(std::string &pw, uint8_t salt[], uint32_t salt_size,
                                uint32_t iteration_count, uint32_t dkLen, bool &aborting_var);

    }
}


#endif //CRYPTOGRAPHY_H
