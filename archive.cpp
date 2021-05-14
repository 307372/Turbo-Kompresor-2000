#include "archive.h"

#include <string>
#include <memory>
#include <fstream>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <utility>


Archive::Archive() : root_folder(std::make_unique<Folder>()) {}


Archive::~Archive()
{
    close();
    // root_folder.release(); // NOLINT(bugprone-unused-return-value)
}


void Archive::close()
{
    if (this->archive_file.is_open())
        this->archive_file.close();
}


void Archive::save( const std::string& path_to_file, bool& aborting_var )
{
    assert(!this->archive_file.is_open());
    this->archive_file.open( path_to_file, std::ios::binary|std::ios::out );
    assert(this->archive_file.is_open());

    char* buffer[1] = {nullptr};
    this->archive_file.write( (char*)buffer, 1 ); // making sure location at byte 0 in file is not valid
    this->root_folder->write_to_archive( this->archive_file, aborting_var );
}


void Archive::load(const std::string& path_to_file )
{
    this->load_path = std::filesystem::path( path_to_file );

    this->archive_file.open( path_to_file, std::ios::binary | std::ios::in | std::ios::out );
    assert( this->archive_file.is_open() );

    this->archive_file.seekg( 1 );
    this->root_folder->parse( this->archive_file, 1, nullptr, this->root_folder );
    this->root_folder->name = std::filesystem::path(path_to_file).filename();
    this->root_folder->name_length = this->root_folder->name.length();
}


void Archive::build_empty_archive() const
{
    this->root_folder->name = "new_archive" + this->extension;
    this->root_folder->name_length = this->root_folder->name.length();
    this->root_folder->location = 1;
}


void Archive::build_empty_archive( std::string archive_name )
{
    this->root_folder->name = archive_name;
    this->root_folder->name_length = this->root_folder->name.length();
    this->root_folder->location = 1;
}


std::unique_ptr<File>* find_file_in_archive( Folder* parent, File* wanted_file ) {
    if ( parent->child_file_ptr.get() == wanted_file ) return &(parent->child_file_ptr);

    std::unique_ptr<File>* tempfile_ptr = &(parent->child_file_ptr);
    while( tempfile_ptr->get() != wanted_file and tempfile_ptr->get() != nullptr ) {
        tempfile_ptr = &(tempfile_ptr->get()->sibling_ptr);
    }
    if ( tempfile_ptr->get() == wanted_file ) return tempfile_ptr;

    return nullptr;
}


std::unique_ptr<Folder>* find_folder_in_archive( Folder* parent, Folder* wanted_folder ) {
    if ( parent->child_dir_ptr.get() == wanted_folder ) return &(parent->child_dir_ptr);

    std::unique_ptr<Folder>* tempfolder_ptr = &(parent->child_dir_ptr);
    while( tempfolder_ptr->get() != wanted_folder and tempfolder_ptr->get() != nullptr ) {
        tempfolder_ptr = &(tempfolder_ptr->get()->sibling_ptr);
    }
    if ( tempfolder_ptr->get() == wanted_folder ) return tempfolder_ptr;
    return nullptr;
}


void Archive::unpack_whole_archive( const std::string& path_to_directory, std::fstream &os, bool& aborting_var )
{
    std::filesystem::path path(path_to_directory);
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories( path ); // create missing folders

    assert(this->archive_file.is_open());

    this->root_folder->unpack( path, os, aborting_var, true );

}


std::unique_ptr<Folder>* Archive::add_folder_to_model( std::unique_ptr<Folder> &parent_dir, const std::string& folder_name )
{
    std::unique_ptr<Folder> *pointer_to_be_returned = nullptr;
    if (parent_dir->child_dir_ptr == nullptr) {
        parent_dir->child_dir_ptr = std::make_unique<Folder>( parent_dir, folder_name );
        pointer_to_be_returned = &(parent_dir->child_dir_ptr);
    }
    else {
        Folder* previous_folder = parent_dir->child_dir_ptr.get();
        while( previous_folder->sibling_ptr != nullptr )
        {
            previous_folder = previous_folder->sibling_ptr.get();
        }
        previous_folder->sibling_ptr = std::make_unique<Folder>( parent_dir, folder_name );
        pointer_to_be_returned = &(previous_folder->sibling_ptr);
    }

    return pointer_to_be_returned;
}


Folder* Archive::add_folder_to_model( Folder* parent_dir, std::string folder_name )
{
    std::unique_ptr<Folder> *pointer_to_be_returned = nullptr;
    if (parent_dir->child_dir_ptr == nullptr) {
        parent_dir->child_dir_ptr = std::make_unique<Folder>( parent_dir, folder_name );
        pointer_to_be_returned = &(parent_dir->child_dir_ptr);
    }
    else {
        Folder* previous_folder = parent_dir->child_dir_ptr.get();
        while( previous_folder->sibling_ptr != nullptr )
        {
            previous_folder = previous_folder->sibling_ptr.get();
        }
        previous_folder->sibling_ptr = std::make_unique<Folder>( parent_dir, folder_name );
        pointer_to_be_returned = &(previous_folder->sibling_ptr);
    }

    return pointer_to_be_returned->get();
}


void Archive::add_file_to_archive_model(std::unique_ptr<Folder>& parent_dir, const std::string& path_to_file, uint16_t& flags )
{
    std::filesystem::path std_path( path_to_file );
    std::unique_ptr<File> new_file = std::make_unique<File>();
    File* ptr_new_file = new_file.get();
    ptr_new_file->path = path_to_file;


    ptr_new_file->name = std_path.filename().string();
    ptr_new_file->name_length = ptr_new_file->name.length();

    ptr_new_file->parent_ptr = parent_dir.get();            // ptr to parent folder in memory
    ptr_new_file->sibling_ptr=nullptr;                      // ptr to next sibling file in memory

    if (parent_dir->child_file_ptr != nullptr)
    {
        File* file_ptr = parent_dir->child_file_ptr.get();  // file_ptr points to previous file
        while( file_ptr->sibling_ptr != nullptr )
        {
            file_ptr = file_ptr->sibling_ptr.get();
        }
        file_ptr->sibling_ptr.swap(new_file);
    }
    else {
        parent_dir->child_file_ptr.swap(new_file);
    }

    ptr_new_file->flags_value = flags;              // 16 flags represented as 16-bit int
    ptr_new_file->data_location = 0;                // location of data in archive (in bytes) will be added to model right before writing the data
    ptr_new_file->compressed_size=0;                // will be determined after compression
    ptr_new_file->original_size = std::filesystem::file_size( std_path );

}


File* Archive::add_file_to_archive_model(Folder &parent_dir, const std::string& path_to_file, uint16_t& flags )
{
    std::filesystem::path std_path( path_to_file );
    std::unique_ptr<File> new_file = std::make_unique<File>();
    File* ptr_new_file = new_file.get();
    ptr_new_file->path = path_to_file;


    ptr_new_file->name = std_path.filename().string();
    ptr_new_file->name_length = ptr_new_file->name.length();

    ptr_new_file->parent_ptr = &parent_dir;                 // ptr to parent folder in memory
    ptr_new_file->sibling_ptr = nullptr;                    // ptr to next sibling file in memory

    if (parent_dir.child_file_ptr.get() != nullptr)
    {
        File* file_ptr = parent_dir.child_file_ptr.get();   // file_ptr points to previous file
        while( file_ptr->sibling_ptr != nullptr )
        {
            file_ptr = file_ptr->sibling_ptr.get();
        }
        file_ptr->sibling_ptr.swap(new_file);
    }
    else {
        parent_dir.child_file_ptr.swap(new_file);
    }

    ptr_new_file->flags_value = flags;                      // 16 flags represented as 16-bit int
    ptr_new_file->data_location = 0;                        // location of data in archive (in bytes) will be added to model right before writing the data
    ptr_new_file->compressed_size=0;                        // will be determined after compression
    ptr_new_file->original_size = std::filesystem::file_size( std_path );

    return ptr_new_file;
}


void Archive::recursive_print() const {
    root_folder->recursive_print( std::cout );
    std::cout << std::endl;
}
