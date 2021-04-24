#ifndef INTEGRITY_VALIDATION_H
#define INTEGRITY_VALIDATION_H

#include <string>


class IntegrityValidation {
public:
    // strings of hex chars
    std::string SHA1;
    std::string SHA256;
    std::string CRC32;

    // arrays of numbers
    uint8_t* SHA1_num;      // 20 bytes
    uint8_t* SHA256_num;    // 32 bytes
    uint8_t* CRC32_num;     // 32 bytes

    IntegrityValidation();
    ~IntegrityValidation();
    std::string get_SHA1_from_file( const std::string& path_to_file, bool& aborting_var );
    std::string get_SHA1_from_stream( std::fstream& source, uint64_t file_size, bool& aborting_var );

    std::string get_SHA256_from_file(const std::string &path_to_file, bool &aborting_var);
    std::string get_SHA256_from_stream( std::fstream& source, bool& aborting_var );
    std::string get_SHA256_from_text( uint8_t text[], uint64_t text_size, bool& aborting_var );

    void generate_CRC32_lookup_table();
    std::string get_CRC32_from_text( uint8_t text[], uint64_t text_size, bool& aborting_var );
    std::string get_CRC32_from_file( std::string path, bool& aborting_var );
    std::string get_CRC32_from_stream( std::fstream& source, bool& aborting_var );
private:
    const uint64_t polynomial = 0x4C11DB7;
    uint32_t CRC32_lookup_table[256];
};

#endif
