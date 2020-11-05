#ifndef STATISTICAL_TOOLS_H
#define STATISTICAL_TOOLS_H
#include <vector>
#include <string>
#include <fstream>

class statistical_tools
{
public:
    unsigned long long max;
    std::ifstream target_file;
    std::vector<unsigned long> r;
    std::string alphabet;
    unsigned long long file_size;
    double entropy;
    statistical_tools( std::string absolute_path_to_file );
    ~statistical_tools();
    void iid_model();
    void iid_model_chunks( size_t buffer_size=1024 );
    void get_entropy();
};

#endif // STATISTICAL_TOOLS_H
