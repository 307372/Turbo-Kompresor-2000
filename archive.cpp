#include "archive.h"

#include <utility>


archive::archive() {
    this->archive_dir = std::make_unique<folder>();
}

archive::~archive()
{
    std::cout << "Destroying the archive..." << std::endl;
    close();
    archive_dir.release(); // NOLINT(bugprone-unused-return-value)
    std::cout << "Destruction of archive succeeded!" << std::endl;
}

void archive::close()
{
    if (this->archive_file.is_open())
        this->archive_file.close();

}

void archive::save( const std::string& path_to_file )
{
    assert(!this->archive_file.is_open());
    this->archive_file.open( path_to_file, std::ios::binary|std::ios::out );
    assert(this->archive_file.is_open());
    char* buffer[1] = {nullptr};
    this->archive_file.write( (char*)buffer, 1 ); // making sure location at byte 0 in file is not valid
    this->archive_dir->write_to_archive(this->archive_file);
}

void archive::load(const std::string& path_to_file )
{
    std::filesystem::path std_path( path_to_file );

    this->archive_file.open( path_to_file, std::ios::binary | std::ios::in );
    assert( this->archive_file.is_open() );

    this->archive_file.seekg( 1 );
    this->archive_dir->parse( this->archive_file, 1, nullptr, this->archive_dir );

}

void archive::build_empty_archive( std::string archive_name = "archive" ) const
{
    this->archive_dir->name = std::move(archive_name);
    this->archive_dir->name_length = this->archive_dir->name.length();
    this->archive_dir->location = 1;
}

void archive::unpack_whole_archive(const std::string& path_to_directory, std::fstream &os) {

    std::filesystem::path path(path_to_directory);
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories( path ); // create missing folders

    assert(this->archive_file.is_open());


    std::cout<<"Commence unpacking!"<<std::endl;
    this->archive_dir->unpack( path, os, true );

}

std::unique_ptr<folder> *archive::add_folder_to_model( std::unique_ptr<folder> &parent_dir, const std::string& folder_name ) {

    folder* added_folder = nullptr;
    std::unique_ptr<folder> *pointer_to_be_returned = nullptr;
    if (parent_dir->child_dir_ptr == nullptr) {
        parent_dir->child_dir_ptr = std::make_unique<folder>( parent_dir, folder_name );
        added_folder = parent_dir->child_dir_ptr.get();
        pointer_to_be_returned = &(parent_dir->child_dir_ptr);
    }
    else {
        folder* previous_folder = parent_dir->child_dir_ptr.get();
        while( previous_folder->sibling_ptr != nullptr )
        {
            previous_folder = previous_folder->sibling_ptr.get();
        }
        previous_folder->sibling_ptr = std::make_unique<folder>( parent_dir, folder_name );
        added_folder = previous_folder->sibling_ptr.get();
        pointer_to_be_returned = &(previous_folder->sibling_ptr);
    }

    return pointer_to_be_returned;
}

void archive::add_file_to_archive_model(std::unique_ptr<folder> &parent_dir, const std::string& path_to_file, uint16_t &flags )
{
    std::filesystem::path std_path( path_to_file );
    std::unique_ptr<file> new_file = std::make_unique<file>();
    file* ptr_new_file = new_file.get();
    ptr_new_file->path = path_to_file;


    ptr_new_file->name = std_path.filename().string();
    ptr_new_file->name_length = ptr_new_file->name.length();


    ptr_new_file->parent_ptr = parent_dir.get();              // ptr to parent folder in memory

    ptr_new_file->sibling_ptr=nullptr;             // ptr to next sibling file in memory

    if (parent_dir->child_file_ptr != nullptr)
    {
        file* file_ptr = parent_dir->child_file_ptr.get(); // file_ptr points to previous file
        while( file_ptr->sibling_ptr != nullptr )
        {
            file_ptr = file_ptr->sibling_ptr.get();
        }
        file_ptr->sibling_ptr.swap(new_file);
    }
    else {
        parent_dir->child_file_ptr.swap(new_file); // potentially breaks everything
        //parent_dir->child_file_location = ptr_new_file->location;
    }

    ptr_new_file->flags_value = flags;                   // 16 flags represented as 16-bit int

    uint32_t size_of_metadata = file::base_metadata_size + ptr_new_file->name_length;
    ptr_new_file->data_location = 0;                 // location of data in archive (in bytes) will be added to model right before writing the data

    ptr_new_file->compressed_size=0; // will be determined after compression
    ptr_new_file->uncompressed_size = std::filesystem::file_size( std_path );

}

void archive::recursive_print() const {
    archive_dir->recursive_print( std::cout );
    std::cout << std::endl;
}


































