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
#include <cmath>

#include "compression.h"
#include "project_exceptions.h"
#include "integrity_validation.h"

struct file;

struct folder
{
    static const uint8_t base_metadata_size = 33;   // base metadata size (excluding name_size) (in bytes)
    uint64_t location=0;                            // absolute location of this folder in archive

    uint8_t name_length=0;                          // length of folder name
    std::string name;                               // folder's name
    std::filesystem::path path;                     // extraction path

    folder* parent_ptr = nullptr;                   // ptr to parent folder in memory

    bool alreadySaved = false;                      // true - file has already been saved to archive, false - it's only in the model
    bool ptr_already_gotten = false;                // true - method get_ptrs was already used on it, so it's in the vector

    std::unique_ptr<folder> child_dir_ptr=nullptr;  // ptr to first subfolder in memory

    std::unique_ptr<folder> sibling_ptr=nullptr;    // ptr to next sibling folder in memory

    std::unique_ptr<file> child_file_ptr=nullptr;   // ptr to first file in memory

    folder( std::unique_ptr<folder> &parent, std::string folder_name );
    folder( folder* parent, std::string folder_name );
    folder();

    void recursive_print(std::ostream &os) const;

    friend std::ostream& operator<<(std::ostream& os, const folder& f);

    void parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<folder> &shared_this  );

    void append_to_archive( std::fstream& archive_file, bool& aborting_var );

    void write_to_archive( std::fstream& archive_file, bool& aborting_var );

    void unpack( const std::filesystem::path& target_path, std::fstream &os, bool& aborting_var, bool unpack_all ) const;

    void get_ptrs( std::vector<folder*>& folders, std::vector<file*>& files );

    void set_path( std::filesystem::path extraction_path, bool set_all_paths );



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
    bool alreadyExtracted = false;                  // true - file has already been extracted
    bool ptr_already_gotten = false;                // true - method get_ptrs was already used on it, so it's in the vector

    std::unique_ptr<file> sibling_ptr=nullptr;      // ptr to next sibling file in memory

    uint16_t flags_value=0;                         // 16 flags represented as 16-bit int

    uint64_t data_location=0;                       // location of data in archive (in bytes)
    uint64_t compressed_size=0;                     // size of compressed data (in bytes)
    uint64_t uncompressed_size=0;                   // size of data before compression (in bytes)

    static const bool dont_abort = false;

    void interpret_flags(std::fstream &os, const std::string& path_to_destination, bool decode, bool& aborting_var, bool validate_integrity = true, uint16_t* progress_ptr = nullptr );

    void recursive_print(std::ostream &os) const;

    friend std::ostream& operator<<(std::ostream &os, const file &f);

    void parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<file> &shared_this );

    void append_to_archive( std::fstream& archive_file, bool& aborting_var, bool write_siblings = true, uint16_t* progress_var = nullptr );

    void write_to_archive( std::fstream& archive_file, bool& aborting_var, bool write_siblings = true, uint16_t* progress_var = nullptr );

    void unpack( const std::string& path, std::fstream &os, bool& aborting_var, bool unpack_all, bool validate_integrity = true, uint16_t* progress_var = nullptr );

    std::string get_compressed_filesize_str(bool scaled);

    std::string get_uncompressed_filesize_str(bool scaled);

    void get_ptrs( std::vector<file*>& files, bool get_siblings_too = false );

    void set_path( std::filesystem::path extraction_path, bool set_all_paths );

};

#endif //EXPERIMENTAL_ARCHIVE_STRUCTURES_H
