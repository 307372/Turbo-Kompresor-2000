#include <iostream>
#include "notcompression.h"
notcompression::notcompression() = default;

uint64_t notcompression::encode( std::fstream &destination, std::string src_path ) {
    std::filesystem::path src( src_path );

    if (std::filesystem::exists(src_path))
    {
        this->uncompressed_size = std::filesystem::file_size( src );
        this->compressed_size = this->uncompressed_size;
        std::ifstream src_stream( src, std::ios::binary );
        destination << src_stream.rdbuf();
        src_stream.close();
    }
    return this->compressed_size;
}

void notcompression::decode( std::fstream &source, std::string dst_path, uint64_t compressed_size ) {
    std::filesystem::path dst( dst_path );

    if (std::filesystem::exists( dst.remove_filename() ))
    {
        std::ofstream dst_stream( dst_path, std::ios::binary );
        char buffer[4*1024];

        uint64_t bits_left = compressed_size;
        uint16_t amount_of_data_to_load = 4*1024;
        if (compressed_size < 4*1024)
            amount_of_data_to_load = compressed_size;

        while( amount_of_data_to_load != 0)
        {
            source.read( &buffer[0], amount_of_data_to_load);
            // for (int i =0; i < amount_of_data_to_load; i++)
            //     std::cout << buffer[i];
            // std::cout << std::endl;
            dst_stream.write( (char*) &buffer, amount_of_data_to_load );
            bits_left -= amount_of_data_to_load;

            if (bits_left != 0)
            {
                if (bits_left <= 4 * 1024) {
                    amount_of_data_to_load = bits_left;
                }
                else
                    amount_of_data_to_load = 4*1024;
            } else {
                amount_of_data_to_load = 0;
            }
        }
        dst_stream.close();
        // delete[] buffer;
    }
    else std::cout << "Path " << dst_path << " does not exist!"<< std::endl;

}