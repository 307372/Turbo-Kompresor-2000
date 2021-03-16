#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "archive_structures.h"


class Archive
{
public:
    Archive();
    ~Archive();

    // Size of buffer used while reading/writing files
    uint32_t buffer_size = 8 * 1024;

    // Extension of created archives
    std::string extension = ".tk2k";

    // Path that was used to load the file (if it was loaded)
    std::filesystem::path load_path;

    // Root folder of archive
    std::unique_ptr<Folder> root_folder;

    // Stream for creating/loading archive
    std::fstream archive_file;

    // Closes archive_file if open
    void close();

    // Saves archive to file
    void save( const std::string& path_to_file, bool& aborting_var );

    // Loads archive from file
    void load( const std::string& path_to_file );

    // Creates empty archive, needs to happen before adding files
    void build_empty_archive() const;                       // default archive name
    void build_empty_archive( std::string archive_name );   // custom archive name

    // Finds and returns pointer to unique_ptr to file
    std::unique_ptr<File>* find_file_in_archive( Folder* parent, File* wanted_file );

    // Finds and returns pointer to unique_ptr to folder
    std::unique_ptr<Folder>* find_folder_in_archive( Folder* parent, Folder* wanted_folder );

    // Unpacks whole archive to path_to_dir
    void unpack_whole_archive( const std::string& path_to_directory, std::fstream &os, bool& aborting_var );

    // Adds information about file to archive's model, needs to happen for compression to be possible
    static void add_file_to_archive_model(std::unique_ptr<Folder> &parent_dir, const std::string& path_to_file, uint16_t &flags );
    File* add_file_to_archive_model(Folder& parent_dir, const std::string& path_to_file, uint16_t& flags );

    // Adds folder to archive's model, and returns pointer to unique pointer to it for future use
    static std::unique_ptr<Folder>* add_folder_to_model( std::unique_ptr<Folder> &parent_dir, const std::string& folder_name );
    Folder* add_folder_to_model( Folder* parent_dir, std::string folder_name );

    // Prints whole archive's useful data onto console
    void recursive_print() const;
};

#endif // ARCHIVE_H



