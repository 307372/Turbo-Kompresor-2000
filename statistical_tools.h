#ifndef STATISTICAL_TOOLS_H
#define STATISTICAL_TOOLS_H
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <numeric>

class statistical_tools
{
public:
    uint64_t max;
    std::ifstream target_file;
    uint64_t r[256]{};
    //std::vector<unsigned long> r;
    std::string alphabet;
    uint64_t file_size;
    double entropy{};
    statistical_tools( std::string& absolute_path_to_file );
    statistical_tools();
    ~statistical_tools();
    void iid_model();
    void iid_model_chunks( size_t buffer_size=1024 );
    void iid_model_chunksV2( size_t buffer_size=1024 );
    void get_entropy();
    void iid_model_from_text( uint8_t text[], uint32_t text_size );
};

#endif // STATISTICAL_TOOLS_H
