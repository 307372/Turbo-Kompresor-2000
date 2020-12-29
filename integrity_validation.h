//
// Created by pc on 03.11.2020.
//

#ifndef TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
#define TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
#include <string>

class integrity_validation {
    std::string SHA1;
public:
    integrity_validation();
    bool is_your_machine_big_endian();
    std::string get_SHA1( const std::string& path_to_file, bool& aborting_var );

    void generate_CRC32_lookup_table();
    std::string get_CRC32_from_text( uint8_t text[], uint64_t text_size, bool& aborting_var );
    std::string get_CRC32_from_file( std::string path, bool& aborting_var );
private:
    const uint64_t polynomial = 0x4C11DB7;
    uint32_t CRC32_lookup_table[256];
};


#endif //TURBO_KOMPRESOR_2000_INTEGRITY_VALIDATION_H
