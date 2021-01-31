#ifndef STATISTICAL_TOOLS_H
#define STATISTICAL_TOOLS_H
#include <string>
#include <fstream>


class statistical_tools
{
public:
    uint64_t max;
    std::ifstream target_file;
    uint64_t r[256]{};
    uint32_t rr[256][256];

    std::string alphabet;
    uint64_t file_size;
    double entropy{};
    statistical_tools( std::string& absolute_path_to_file );
    statistical_tools();
    ~statistical_tools();
    void iid_model_chunksV2( size_t buffer_size=1024 );
    void get_entropy();
    void iid_model_from_text( uint8_t text[], uint32_t text_size );
    void model_from_text_1back( uint8_t text[], uint32_t text_size );
    void count_symbols_from_text_0back( uint8_t text[], uint32_t text_size );
    void count_symbols_from_text_1back( uint8_t text[], uint32_t text_size );
};

#endif // STATISTICAL_TOOLS_H
