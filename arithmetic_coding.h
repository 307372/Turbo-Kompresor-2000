#ifndef ARITHMETIC_CODING_H
#define ARITHMETIC_CODING_H
#include <string>
#include <vector>

class arithmetic_coding
{
public:
    int precision;
    unsigned long long whole;
    long double wholed;
    unsigned long long half;
    unsigned long long quarter;
    arithmetic_coding();
    std::string encode_file( std::string X, std::vector<unsigned long> r, std::string path, size_t buffer_size );
    void decode_to_file(std::string encoded_message, std::vector<unsigned long> r, std::string alphabet, std::string path, unsigned long long file_size);
};

#endif // ARITHMETIC_CODING_H
