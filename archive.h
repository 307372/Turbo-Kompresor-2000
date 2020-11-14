#ifndef ARCHIVE_H
#define ARCHIVE_H
#include <string>
#include <memory>
#include <any>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <iostream>
#include "notcompression.h"
#include "archive_structures.h"

class archive
{
public:
    archive();
    ~archive();

    // Size of buffer used while reading/writing files
    const uint32_t buffer_size = 8 * 1024;

    // Extension of created archives
    const std::string extension= ".tk2k";

    // Root folder of archive
    std::unique_ptr<folder> archive_dir;

    // Stream for creating/loading archive
    std::fstream archive_file;

    // Closes archive_file if open
    void close();

    // Saves archive
    void save( const std::string& path_to_file );

    // Loads archive from file
    void load(const std::string& path_to_file );

    // Creates empty archive, needs to happen before adding files
    void build_empty_archive( std::string archive_name) const;

    // Unpacks whole archive to path_to_dir
    void unpack_whole_archive( const std::string& path_to_directory, std::fstream &os );

    // Adds information about file to archive's model, needs to happen for compression to be possible
    static void add_file_to_archive_model(std::unique_ptr<folder> &parent_dir, const std::string& path_to_file, uint16_t &flags );

    // Adds folder to archive's model, and returns pointer to unique pointer to it for future use
    static std::unique_ptr<folder>* add_folder_to_model( std::unique_ptr<folder> &parent_dir, const std::string& folder_name );

    // Prints whole archive's useful data onto console
    void recursive_print() const;
};

#endif // ARCHIVE_H



