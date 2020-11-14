//
// Created by pc on 09.11.2020.
//

#ifndef EXPERIMENTAL_NOTCOMPRESSION_H
#define EXPERIMENTAL_NOTCOMPRESSION_H
#include <fstream>
#include <string>
#include <filesystem>
#include <cassert>
// For testing purposes only


class notcompression {
public:
    notcompression();
    uint64_t compressed_size = 0;
    uint64_t uncompressed_size = 0;
    uint64_t encode( std::fstream &destination, std::string src_path );
    void decode( std::fstream &source, std::string dst_path, uint64_t compressed_size );

};


#endif //EXPERIMENTAL_NOTCOMPRESSION_H
