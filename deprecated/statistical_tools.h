#ifndef STATISTICAL_TOOLS_H
#define STATISTICAL_TOOLS_H

#include <string>
#include <fstream>


class StatisticalTools
{
public:
    uint64_t max;
    std::ifstream target_file;
    uint64_t r[256]{};
    uint32_t rr[256][256];

    std::string alphabet;
    uint64_t file_size;
    double entropy{};
    StatisticalTools( std::string& absolute_path_to_file );
    StatisticalTools();
    ~StatisticalTools();
    void model_from_text_0back( uint8_t text[], uint32_t text_size );
    void model_from_text_1back( uint8_t text[], uint32_t text_size );

    /*
    void get_entropy();                                                         // never used
    void iid_model_chunksV2( size_t buffer_size=1024 );                         // never used
    void count_symbols_from_text_0back( uint8_t text[], uint32_t text_size );   // never used
    void count_symbols_from_text_1back( uint8_t text[], uint32_t text_size );   // never used
    */
};

#endif // STATISTICAL_TOOLS_H
