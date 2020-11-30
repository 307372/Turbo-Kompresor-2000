#ifndef EXPERIMENTAL_ARCHIVE_STRUCTURES_H
#define EXPERIMENTAL_ARCHIVE_STRUCTURES_H
#include <string>
#include <memory>
#include <any>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <bitset>
#include <utility>

//#include "arithmetic_coding.h"
#include "compression.h"
#include "notcompression.h"
#include "project_exceptions.h"
#include "integrity_validation.h"

struct file;

struct folder
{
    static const uint8_t base_metadata_size = 33;   // base metadata size (excluding name_size) (in bytes)
    uint64_t location=0;                            // absolute location of this folder in archive

    uint8_t name_length=0;                          // length of folder name
    std::string name;                               // folder's name

    folder* parent_ptr = nullptr;                   // ptr to parent folder in memory

    bool alreadySaved = false;                      // true - file has already been saved to archive, false - it's only in the model

    std::unique_ptr<folder> child_dir_ptr=nullptr;  // ptr to first subfolder in memory

    std::unique_ptr<folder> sibling_ptr=nullptr;    // ptr to next sibling folder in memory

    std::unique_ptr<file> child_file_ptr=nullptr;   // ptr to first file in memory

    folder( std::unique_ptr<folder> &parent, std::string folder_name );
    folder( folder* parent, std::string folder_name );
    folder();

    void recursive_print(std::ostream &os) const;

    friend std::ostream& operator<<(std::ostream& os, const folder& f);

    void parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<folder> &shared_this  );

    void append_to_archive( std::fstream& archive_file );

    void write_to_archive( std::fstream &archive_file );

    void unpack( const std::filesystem::path& target_path, std::fstream &os, bool unpack_all ) const;



};

struct file
{
    static const uint8_t base_metadata_size = 43;   // base metadata size (excluding name_size) (in bytes)
    std::string path;

    uint64_t location=0;                            // absolute location of this file in archive (starts at name_length)
    uint8_t name_length=0;                          // length of file name (in bytes)
    std::string name;                               // folder's name

    folder* parent_ptr = nullptr;                   // ptr to parent folder in memory

    bool alreadySaved = false;                      // true - file has already been saved to archive, false - it's only in the model

    std::unique_ptr<file> sibling_ptr=nullptr;      // ptr to next sibling file in memory

    uint16_t flags_value=0;                         // 16 flags represented as 16-bit int

    uint64_t data_location=0;                       // location of data in archive (in bytes)
    uint64_t compressed_size=0;                     // size of data (in bytes)
    uint64_t uncompressed_size=0;                   // size of compressed data in _B_I_T_S_

    void interpret_flags(std::fstream &os, const std::string& path_to_destination, bool decode );

    void recursive_print(std::ostream &os) const;

    friend std::ostream& operator<<(std::ostream &os, const file &f);

    void parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<file> &shared_this );

    void append_to_archive( std::fstream& archive_file );

    void write_to_archive( std::fstream &archive_file );

    void unpack( const std::string& path, std::fstream &os, bool unpack_all );


    // void interpret_flags( uint16_t flags );
};

#endif //EXPERIMENTAL_ARCHIVE_STRUCTURES_H
