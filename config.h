#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <fstream>

class Config
{
public:
    Config();
    ~Config();

    void parse();
    void save_default();

    std::filesystem::path get_extraction_path() { return extraction_path; }
    void set_extraction_path( std::filesystem::path path ) { extraction_path = path; save(); }

    bool get_filesize_scaling() { return filesize_scaling; }
    void set_filesize_scaling( bool filesize_scaling ) { this->filesize_scaling = filesize_scaling; save(); }
private:
    void save();

    std::fstream config_file;

    std::filesystem::path extraction_path;
    bool filesize_scaling;

};

#endif // CONFIG_H
