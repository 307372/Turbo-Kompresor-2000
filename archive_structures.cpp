#include "archive_structures.h"

#include <QApplication>
#include <iostream>
#include <bitset>


#include "misc/multithreading.h"
#include "cryptography.h"

File::~File() {
    if (!key.empty()) { // making sure key is erased from memory
        for (auto& byte : key) {
            byte = 0;
        }
    }
}



bool File::process_the_file(std::fstream &archive_stream, const std::string& path_to_destination, bool encode, bool& aborting_var, bool validate_integrity, uint16_t* progress_ptr ) {

    assert(archive_stream.is_open());

    bool successful = false;
    std::bitset<16> flags_bin(this->flags_value);
    bool is_encrypted = flags_bin[6];

    if (encode == true)
    {
        if (is_encrypted) {
            assert(!key.empty());
            assert(!encryption_metadata.empty());

            auto* copied_key = new uint8_t[key.length()];
            for (uint16_t i=0; i < key.length(); ++i) {
                copied_key[i] = (uint8_t)key[i];
            }

            successful = multithreading::processing_foreman(archive_stream, this->path, multithreading::mode::compress,
                                                            flags_value, original_size, &this->compressed_size,
                                                            aborting_var, validate_integrity, progress_ptr,
                                                            &copied_key, // randomly generated key
                                                            const_cast<uint8_t*>(encryption_metadata.c_str()), // actual metadata
                                                            encryption_metadata.length() ); // actual metadata length
            delete[] copied_key;
        }
        else {
                successful = multithreading::processing_foreman(archive_stream, this->path, multithreading::mode::compress,
                                                                flags_value, original_size, &this->compressed_size,
                                                                aborting_var, validate_integrity, progress_ptr );
        }
    }
    else
    {
        archive_stream.seekg(this->data_location);

        if (is_encrypted) {
            uint8_t* empty_metadata_ptr = nullptr;
            uint64_t whatever_metadata_size = 0;

            assert(!key.empty());
            assert(!encryption_metadata.empty());

            auto pw_key = new uint8_t [key.length()];
            for (uint16_t i=0; i < key.length(); ++i) {
                pw_key[i] = key[i];
            }

            successful = multithreading::processing_foreman(archive_stream, path_to_destination + '/' + this->name,
                                                            multithreading::mode::decompress, flags_value,
                                                            original_size, &this->compressed_size, aborting_var,
                                                            validate_integrity, progress_ptr,
                                                            &pw_key, // PBKDF2(password)
                                                            empty_metadata_ptr,
                                                            whatever_metadata_size );
            delete[] empty_metadata_ptr;
            delete[] pw_key;
        }
        else {
            successful = multithreading::processing_foreman(archive_stream, path_to_destination + '/' + this->name,
                                                            multithreading::mode::decompress, flags_value,
                                                            original_size, &this->compressed_size, aborting_var,
                                                            validate_integrity, progress_ptr);
        }
    }
    return successful;
}


void File::recursive_print(std::ostream &os) const {
    os << *this << '\n';
    if (sibling_ptr) sibling_ptr->recursive_print( os );
}


std::ostream& operator<<(std::ostream &os, const File &f)
{
    os << "File named: \"" << f.name << "\", len(name) = " << f.name_length << '\n';
    os << "Has flags " << f.flags_value << '\n';
    os << "Header starts at byte " << f.location << ", with total size of " << File::base_metadata_size + f.name_length << " bytes\n";
    os << "Compressed data of this file starts at byte " << f.data_location << "\n";
    assert(f.parent_ptr);
    os << "Parent located at byte " << f.parent_ptr->location << ", ";
    if (f.sibling_ptr) os << "Sibling located at byte " << f.sibling_ptr->location << '\n';
    else os << "there's no sibling\n";
    os << "With compressed size of " << f.compressed_size << " bits, and uncompressed size of " << f.original_size << " bytes." << std::endl;
    return os;
}


void File::parse( std::fstream &os, uint64_t pos, Folder* parent ) {
    uint8_t buffer[8];
    os.seekg( pos );

    this->alreadySaved = true;
    this->location = pos;
    this->name_length = (uint8_t)os.get();

    this->name = std::string(name_length, ' '); // initialising space-filled name_length-long string
    os.read( &this->name[0], name_length );


    // Getting location of parent folder in the archive
    os.read( (char*)buffer, 8 );
    uint64_t parent_location_test = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    if (parent_location_test !=0) this->parent_ptr = parent;


    // Getting location of sibling file in the archive
    os.read( (char*)buffer, 8 );
    uint64_t sibling_location_pos = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    // Getting flags for this file from the archive
    os.read( (char*)buffer, 2 );
    this->flags_value = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u);

    std::bitset<16> bin_flags(flags_value);
    if (bin_flags[6]) { // checking if the file is encrypted
        this->encrypted = true;
        this->locked = true;
    }


    // Getting location of compressed data for this file from the archive
    os.read( (char*)buffer, 8 );
    this->data_location = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    // Getting size of compressed data of this file from the archive (in   B I T S)
    os.read( (char*)buffer, 8 );
    this->compressed_size = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    // Getting size of uncompressed data of this file from the archive
    os.read( (char*)buffer, 8 );
    this->original_size = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    if (sibling_location_pos != 0) {  // if there's another file in this dir, parse it too
        this->sibling_ptr = std::make_unique<File>();
        uint64_t backup_g = os.tellg();
        this->sibling_ptr->parse(os, sibling_location_pos, parent);
        os.seekg( backup_g );
    }
}


bool File::write_to_archive( std::fstream &archive_file, bool& aborting_var, bool write_siblings, uint16_t* progress_var ) {
    bool successful = false;
    if (!this->alreadySaved and !aborting_var) {
        this->alreadySaved = true;
        location = archive_file.tellp();

        if (parent_ptr != nullptr) // correcting current file's location in model, and in the file
        {
            if ( parent_ptr->child_file_ptr.get() == this ) {

                uint64_t backup_p = archive_file.tellp();

                archive_file.seekp( parent_ptr->location + 1 + parent_ptr->name_length + 24 ); // seekp( start of child_file_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu;

                archive_file.write((char*)buffer, 8);
                delete[] buffer;

                archive_file.seekp( backup_p );

            }
            else {
                File* file_ptr = parent_ptr->child_file_ptr.get(); // location of previous file in the same dir
                while( file_ptr->sibling_ptr != nullptr )
                {
                    if (this != file_ptr->sibling_ptr.get())
                        file_ptr = file_ptr->sibling_ptr.get();
                    else
                        break;
                }


                uint64_t backup_p = archive_file.tellp();
                archive_file.seekp( file_ptr->location + 1 + file_ptr->name_length + 8 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu;

                archive_file.write((char*)buffer, 8);
                delete[] buffer;
                archive_file.seekp( backup_p );
            }
        }


        uint32_t buffer_size = base_metadata_size + name_length;
        auto buffer = new uint8_t[buffer_size];
        uint32_t bi=0; //buffer index

        buffer[bi] = name_length;   //writing name length to file
        bi++;

        for (uint16_t i=0; i < name_length; i++)    //writing name to file
            buffer[bi+i] = name[i];
        bi += name_length;

        if (parent_ptr)
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = (parent_ptr->location >> (i * 8u)) & 0xFFu;
        else
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
        bi += 8;

        if (sibling_ptr)
            for (uint8_t i=0; i < 8; i++)
                buffer[bi+i] = (sibling_ptr->location >> (i * 8u)) & 0xFFu;
        else
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
        bi+=8;

        for (uint8_t i=0; i < 2; i++)
            buffer[bi+i] = ((unsigned)flags_value >> (i * 8u)) & 0xFFu;
        bi+=2;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (data_location >> (i * 8u)) & 0xFFu;
        bi+=8;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (compressed_size >> (i * 8u)) & 0xFFu;
        bi+=8;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (original_size >> (i * 8u)) & 0xFFu;
        bi+=8;

        assert( bi == buffer_size );
        archive_file.write((char*)buffer, buffer_size);
        delete[] buffer;


        auto backup_end_of_metadata = archive_file.tellp(); // position in file right after the end of metadata

        data_location = backup_end_of_metadata;

        // encoding
        successful = process_the_file( archive_file, "encoding has it's path in the file object", true, aborting_var, true, progress_var );

        auto backup_p = archive_file.tellp();

        archive_file.seekp( data_location - 24 );

        auto buffer2 = new uint8_t[16];

        for (uint8_t i=0; i < 8; i++)
            buffer2[i] = (data_location >> (i*8u)) & 0xFFu;
        for (uint8_t i=0; i < 8; i++)
            buffer2[i+8] = (compressed_size >> (i*8u)) & 0xFFu;

        archive_file.write((char*)buffer2, 16);
        delete[] buffer2;

        archive_file.seekp( backup_p );
    }

    if (sibling_ptr and write_siblings and !aborting_var) sibling_ptr->write_to_archive( archive_file, aborting_var, write_siblings );
    return successful;
}


bool File::unpack( const std::string& path_to_destination, std::fstream &os, bool& aborting_var, bool unpack_all, bool validate_integrity, uint16_t* progress_var )
{
    uint64_t backup_g = os.tellg();
    os.seekg( this->data_location );

    bool success = process_the_file( os, path_to_destination, false, aborting_var, validate_integrity, progress_var );

    os.seekg( backup_g );

    if (sibling_ptr != nullptr and unpack_all)
    {
        sibling_ptr->unpack( path_to_destination, os, aborting_var, unpack_all, validate_integrity, progress_var );
    }

    return success;
}


std::string File::get_compressed_filesize_str(bool scaled) {
    std::string units[5] = {"B","KB","MB","GB","TB"};

    float filesize = this->compressed_size;
    uint16_t unit_i = 0;
    if (scaled) {
        for (uint16_t i=0; i<5; ++i) {
            if (filesize > 1000) {
                filesize /= 1000;
                unit_i++;
            }
            else break;
        }
    }
    if ( unit_i != 0 ) {
        std::string output = std::to_string(std::lround(filesize*10));
        output.insert(output.length()-1, 1, '.');

        return output + ' ' + units[unit_i];
    }
    else    // unit == bytes
    {
        if ( filesize < 1000 ) return std::to_string(std::lround(filesize)) + " " + units[unit_i];
        else {
            std::string bytes = std::to_string((uint64_t)filesize);
            std::string output;

            for (uint64_t i=0; i < bytes.length(); ++i)
            {
                output += bytes[i];
                if ((bytes.length()-i-1)%3==0 and i != bytes.length()-1) output +=',';
            }
            return output + ' ' + units[unit_i];
        }
    }
}


std::string File::get_uncompressed_filesize_str(bool scaled) {
    std::string units[5] = {"B","KB","MB","GB","TB"};

    float filesize = this->original_size;
    uint16_t unit_i = 0;
    if (scaled) {
        for (uint16_t i=0; i<5; ++i) {
            if (filesize > 1000) {
                filesize /= 1000;
                unit_i++;
            }
            else break;
        }
    }
    if ( unit_i != 0 ) {
        std::string output = std::to_string(std::lround(filesize*10));
        output.insert(output.length()-1, 1, '.');

        return output + ' ' + units[unit_i];
    }
    else    // unit == bytes
    {
        if ( filesize < 1000 ) return std::to_string(std::lround(filesize)) + " " + units[unit_i];
        else {
            std::string bytes = std::to_string((uint64_t)filesize);
            std::string output;

            for (uint64_t i=0; i < bytes.length(); ++i)
            {
                output += bytes[i];
                if ((bytes.length()-i-1)%3==0 and i != bytes.length()-1) output +=',';
            }
            return output + ' ' + units[unit_i];
        }
    }
}


bool File::append_to_archive( std::fstream& archive_file, bool& aborting_var, bool write_siblings, uint16_t* progress_var ) {
    archive_file.seekp(0, std::ios_base::end);
    return this->write_to_archive( archive_file, aborting_var, write_siblings, progress_var );
}


void File::get_ptrs( std::vector<File*>& files, bool get_siblings_too ) {
    if ( !this->ptr_already_gotten ) {
        files.emplace_back( this );
        this->ptr_already_gotten = true;
    }

    if ( this->sibling_ptr.get() != nullptr and get_siblings_too ) this->sibling_ptr->get_ptrs( files, get_siblings_too );
}


void File::set_path( std::filesystem::path extraction_path, bool set_all_paths ) {
    if ( this->ptr_already_gotten ) this->path = extraction_path;

    if ( this->sibling_ptr.get() != nullptr and set_all_paths ) this->sibling_ptr->set_path( extraction_path, set_all_paths );
}


void File::copy_to_another_archive( std::fstream& src, std::fstream& dst, uint64_t parent_location, uint64_t previous_sibling_location, uint16_t previous_name_length )
{
    if (!this->ptr_already_gotten) {    // if ptr_already_gotten, don't copy this

        assert(parent_location != 0);

        dst.seekp(0, std::ios_base::end);
        uint64_t dst_location = dst.tellp();

        src.seekg(this->location);

        if (previous_sibling_location == 0)
        {
            dst.seekp( parent_location + 1 + parent_ptr->name_length + 24 ); // seekp( start of child_file_location in archive file )
            dst.write((char*)&dst_location, 8);

        }
        else
        {
            assert(previous_name_length != 0 and previous_name_length < 256);
            dst.seekp( previous_sibling_location + 1 + previous_name_length + 8 ); // seekp( start of sibling_location in archive file )
            dst.write((char*)&dst_location, 8);
        }

        dst.seekp(0, std::ios_base::end);

        uint32_t buffer_size = base_metadata_size+name_length;
        auto buffer = new uint8_t[buffer_size];
        uint32_t bi=0; //buffer index

        // (name length)
        buffer[bi] = name_length;
        bi++;

        // (file name)
        for (uint16_t i=0; i < name_length; i++)
            buffer[bi+i] = name[i];
        bi += name_length;

        // (parent_ptr)
        for (uint8_t i = 0; i < 8; i++)
            buffer[bi + i] = (parent_location >> (i * 8u)) & 0xFFu;
        bi += 8;

        // (sibling_ptr)
        for (uint8_t i = 0; i < 8; i++)
            buffer[bi + i] = 0;
        bi+=8;

        // (flags)
        for (uint8_t i=0; i < 2; i++)
            buffer[bi+i] = ((unsigned)flags_value >> (i * 8u)) & 0xFFu;
        bi+=2;

        // (location of data)
        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = ( (dst_location - this->location + this->data_location) >> (i * 8u)) & 0xFFu;    // potentially broken
        bi+=8;

        // (compressed size)
        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (compressed_size >> (i * 8u)) & 0xFFu;
        bi+=8;

        // (original size)
        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (original_size >> (i * 8u)) & 0xFFu;
        bi+=8;


        assert( bi == buffer_size );
        dst.write((char*)buffer, buffer_size);
        delete[] buffer;

        assert( dst.tellp() == dst_location - this->location + this->data_location );

        assert(this->data_location != 0);
        src.seekg(this->data_location);

        // copying encoded data + checksum

        uint64_t total_data_size = this->compressed_size;
        if ((this->flags_value & (1<<15))>>15) // if it's got SHA-1 appended
        {
            total_data_size += 40;  // length of SHA-1 in my file (bytes)
        }
        else if ((this->flags_value & (1<<14))>>14) // if it's got CRC-32 appended
        {
            total_data_size += 10;  // length of CRC-32 in my file (bytes)
        }

        uint32_t output_buffer_size = 4*8*1024;
        auto output_buffer = new uint8_t[output_buffer_size];

        while ( total_data_size > output_buffer_size )
        {
            src.read( (char*)output_buffer, output_buffer_size );
            total_data_size -= output_buffer_size;
            dst.write( (char*)output_buffer, output_buffer_size );
        }

        src.read( (char*)output_buffer, total_data_size );
        dst.write( (char*)output_buffer, total_data_size );

        delete[] output_buffer;


        if (sibling_ptr) sibling_ptr->copy_to_another_archive(src, dst, parent_location, dst_location, this->name_length);
    }
    else if (sibling_ptr){
        assert(previous_sibling_location < UINT32_MAX);
        sibling_ptr->copy_to_another_archive(src, dst, parent_location, previous_sibling_location, previous_name_length);
    }
}


bool File::is_locked() const {return locked;}


bool File::is_encrypted() const {return encrypted;}


void File::prepare_for_encryption(std::string& pw, bool& aborting_var)
// prepares metadata and random key for the purpose of encrypting the file
{
    // changing coursor & trying to unlock the file
    reinterpret_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::WaitCursor);

    uint32_t metadata_size = 88;
    auto metadata = new uint8_t [metadata_size];

    // generating salt
    uint32_t salt_size = crypto::PBKDF2::saltSize;
    auto* salt = new uint8_t [salt_size];
    std::mt19937 unsafe_gen(std::random_device{}());
    // salt doesn't need to be cryptographically secure
    crypto::PRNG::fill_with_random_data(salt, salt_size, unsafe_gen);

    // genearting key from password
    uint32_t key_size = crypto::AES128::key_size;
    auto iteration_count = static_cast<uint64_t>(crypto::PBKDF2::iteration_count::low);
    std::string pw_key = crypto::PBKDF2::HMAC_SHA256(pw, salt, salt_size,
                                                     iteration_count, key_size, aborting_var);

    // generating random key
    uint32_t random_key_size = key_size;
    auto* random_key = new uint8_t [random_key_size];
    CryptoPP::AutoSeededX917RNG<CryptoPP::AES> crypto_gen;
    crypto::CSPRNG::fill_with_random_data(random_key, random_key_size, crypto_gen);

    crypto::AES128::generate_metadata((uint8_t*) pw_key.c_str(), pw_key.length(), salt, salt_size, iteration_count,
                                      random_key, random_key_size, metadata, metadata_size, unsafe_gen, aborting_var);

    key = std::basic_string<uint8_t>(random_key, random_key_size);
    encryption_metadata = std::basic_string<uint8_t>(metadata, metadata_size);

    assert(key.length() != 0);
    assert(encryption_metadata.length() != 0);

    this->encrypted = true;

    delete[] random_key;
    delete[] salt;
    delete[] metadata;

    // restoring original coursor
    reinterpret_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();
}


bool File::unlock(std::string& pw, std::fstream& archive_stream, bool& aborting_var)
// Checks whether or not the provided password is correct, and if it is, saves PBKDF2(pw) for future use
{
    assert(locked);
    assert(archive_stream.is_open());

    // changing coursor & trying to unlock the file
    reinterpret_cast<QGuiApplication*>(QApplication::instance())->setOverrideCursor(Qt::WaitCursor);

    std::streamoff original_pos = archive_stream.tellg();
    archive_stream.seekg(data_location+4+4); // we'll need to skip multithreading metadata, that's why there's +4+4

    uint32_t metadata_size = 88;
    auto* metadata = new uint8_t [metadata_size];
    archive_stream.read(reinterpret_cast<char *>(metadata), metadata_size);
    archive_stream.seekg(original_pos);

    uint64_t iteration_count = *((uint64_t*)(metadata + 16));
    std::string pwkey = crypto::PBKDF2::HMAC_SHA256(pw, metadata, crypto::PBKDF2::saltSize,
                                                    iteration_count, crypto::AES128::key_size,
                                                    aborting_var);

    bool is_pw_correct = crypto::AES128::verify_password_key((uint8_t*)pwkey.c_str(), pwkey.length(), metadata,
                                                             metadata_size, aborting_var);

    // restoring original coursor
    reinterpret_cast<QGuiApplication*>(QApplication::instance())->restoreOverrideCursor();

    if (is_pw_correct) {
        this->key = std::basic_string<uint8_t>((uint8_t*)pwkey.c_str(), pwkey.length());
        this->encryption_metadata = std::basic_string<uint8_t>(metadata, metadata_size);

        this->locked = false;

        delete[] metadata;
        return true;
    }
    else {
        delete[] metadata;
        return false;
    }

}


// Folder methods below


Folder::Folder()= default;


Folder::Folder( std::unique_ptr<Folder> &parent, std::string folder_name ) :
    name_length(folder_name.length()),
    name(std::move(folder_name)),
    parent_ptr(parent.get()) {}


Folder::Folder( Folder* parent, std::string folder_name ) :
    name_length(folder_name.length()),
    name(std::move(folder_name)),
    parent_ptr(parent) {}


void Folder::recursive_print(std::ostream &os) const {
    os << *this << '\n';
    if (child_file_ptr) child_file_ptr->recursive_print( os );
    if (sibling_ptr) sibling_ptr->recursive_print( os );
    if (child_dir_ptr) child_dir_ptr->recursive_print( os );
}


std::ostream& operator<<(std::ostream& os, const Folder& f){
    os << "Folder named: \"" << f.name << "\", len(name) = " << (uint32_t)f.name_length << '\n';
    os << "Header starts at byte " << f.location << ", with total size of " << Folder::base_metadata_size + f.name_length << " bytes\n";
    if (f.parent_ptr) os << "Parent located at byte " << f.parent_ptr->location << ", ";
    else os << "There's no parent, ";
    if (f.sibling_ptr) os << "Sibling located at byte " << f.sibling_ptr->location << '\n';
    else os << "there's no sibling\n";

    if (f.child_dir_ptr) os << "Child folder located at byte " << f.child_dir_ptr->location << ", ";
    else os << "There's no child folder, ";
    if (f.child_file_ptr) os << "child file located at byte " << f.child_file_ptr->location << std::endl;
    else os << "there's no child file." << std::endl;

    return os;
}


void Folder::parse( std::fstream &os, uint64_t pos, Folder* parent, std::unique_ptr<Folder> &shared_this  )
{
    uint8_t buffer[8];
    os.seekg( pos );

    this->alreadySaved = true;
    this->location = pos;
    this->name_length = (uint8_t)os.get();

    this->name = std::string(name_length, ' '); // initialising empty string name_length-long
    os.read( &this->name[0], name_length );

    os.read( (char*)buffer, 8 );
    uint64_t parent_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);

    if (parent_pos_in_file !=0) this->parent_ptr = parent;


    os.read( (char*)buffer, 8 );
    uint64_t child_dir_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);



    if (child_dir_pos_in_file !=0) {
        this->child_dir_ptr = std::make_unique<Folder>();
        uint64_t backup_g = os.tellg();
        this->child_dir_ptr->parse(os, child_dir_pos_in_file, shared_this.get(), child_dir_ptr );
        os.seekg( backup_g );
    }


    os.read( (char*)buffer, 8 );
    uint64_t sibling_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);

    if (sibling_pos_in_file != 0) {
        this->sibling_ptr = std::make_unique<Folder>();
        uint64_t backup_g = os.tellg();
        this->sibling_ptr->parse(os, sibling_pos_in_file, parent, sibling_ptr);
        os.seekg( backup_g );
    }


    os.read( (char*)buffer, 8 );
    uint64_t child_file_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    if (child_file_pos_in_file != 0) {
        this->child_file_ptr = std::make_unique<File>();
        uint64_t backup_g = os.tellg();
        this->child_file_ptr->parse(os, child_file_pos_in_file, shared_this.get() ); // Seemingly implemented
        os.seekg( backup_g );
    }
}


void Folder::append_to_archive( std::fstream& archive_file, bool& aborting_var ) {
    archive_file.seekp(0, std::ios_base::end);
    this->write_to_archive( archive_file, aborting_var );
}


void Folder::write_to_archive( std::fstream &archive_file, bool& aborting_var ) {

    if (!this->alreadySaved) {
        this->alreadySaved = true;
        location = archive_file.tellp();

        if (parent_ptr != nullptr) { // correcting current dir's location in model, and in file
            if ( parent_ptr->child_dir_ptr.get() == this ) {
                // updating parent's knowledge of it's firstborn's location in file
                uint64_t backup_p = archive_file.tellp();

                archive_file.seekp( parent_ptr->location + 1 + parent_ptr->name_length + 8 ); // seekp( start of child_dir_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu;

                archive_file.write((char*)buffer, 8);
                delete[] buffer;

                archive_file.seekp( backup_p );
            }
            else {
                Folder* previous_folder = parent_ptr->child_dir_ptr.get();
                while( previous_folder->sibling_ptr != nullptr )
                {
                    if (this != previous_folder->sibling_ptr.get())
                        previous_folder = previous_folder->sibling_ptr.get();
                    else
                        break;
                }

                uint64_t backup_p = archive_file.tellp();
                archive_file.seekp( previous_folder->location + 1 + previous_folder->name_length + 16 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu;

                archive_file.write((char*)buffer, 8);
                delete[] buffer;

                archive_file.seekp( backup_p );
            }
        }

        uint32_t buffer_size = base_metadata_size+name_length;
        auto buffer = new uint8_t[buffer_size];
        uint32_t bi=0; //buffer index

        buffer[bi] = name_length;   //writing name length to file
        bi++;

        for (uint16_t i=0; i < name_length; i++)    //writing name to file
            buffer[bi+i] = name[i];
        bi += name_length;

        if (parent_ptr)
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = (parent_ptr->location >> (i * 8u)) & 0xFFu;
        else
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
        bi += 8;

        if (child_dir_ptr)
            for (uint8_t i=0; i < 8; i++)
                buffer[bi+i] = (child_dir_ptr->location >> (i*8u)) & 0xFFu;
        else
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
        bi+=8;

        if (sibling_ptr) {
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = (sibling_ptr->location >> (i * 8u)) & 0xFFu;
            bi += 8;
        } else {
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
            bi += 8;
        }

        if (child_file_ptr)
            for (uint8_t i=0; i < 8; i++)
                buffer[bi+i] = (child_file_ptr->location >> (i*8u)) & 0xFFu;
        else
            for (uint8_t i=0; i < 8; i++)
                buffer[bi+i] = 0;

        assert( bi+8 == buffer_size );
        archive_file.write((char*)buffer, buffer_size);
        delete[] buffer;
    }

    if (sibling_ptr)
        sibling_ptr->write_to_archive( archive_file, aborting_var );

    if (child_file_ptr)
        child_file_ptr->write_to_archive( archive_file, aborting_var );

    if (child_dir_ptr)
        child_dir_ptr->write_to_archive( archive_file, aborting_var );

}


void Folder::unpack( const std::filesystem::path& target_path, std::fstream &os, bool& aborting_var, bool unpack_all ) const
{
    std::string temp_name;
    if (this->parent_ptr == nullptr) temp_name = std::filesystem::path(this->name).stem();
    else temp_name = this->name;

    std::filesystem::path path_with_this_folder( target_path.string() + '/' + temp_name );
    if ( !std::filesystem::exists(path_with_this_folder) )
        std::filesystem::create_directories( path_with_this_folder );

    if (unpack_all) {
        if( sibling_ptr != nullptr )
            sibling_ptr->unpack(target_path, os, aborting_var, unpack_all);
        if( child_dir_ptr != nullptr )
            child_dir_ptr->unpack( path_with_this_folder, os, aborting_var, unpack_all);
        if( child_file_ptr != nullptr )
            child_file_ptr->unpack( path_with_this_folder, os, aborting_var, unpack_all);
    }

}


void Folder::get_ptrs( std::vector<Folder*>& folders, std::vector<File*>& files ) {
    if ( !this->ptr_already_gotten ) {
        folders.emplace_back( this );
        this->ptr_already_gotten = true;
    }

    if (this->child_dir_ptr.get() != nullptr) this->child_dir_ptr->get_ptrs( folders, files );
    if (this->child_file_ptr.get() != nullptr) this->child_file_ptr->get_ptrs( files, true );

}


void Folder::set_path( std::filesystem::path extraction_path, bool set_all_paths ) {
    auto folder_path = extraction_path;

    if ( this->ptr_already_gotten ) {
        if (this->parent_ptr == nullptr)    // if this is main archive folder
        {
            std::filesystem::path extensionless_name = std::filesystem::path(this->name).stem();
            folder_path /= extensionless_name;
        }
        else
            folder_path.append(this->name);
        this->path = folder_path;
    }

    if (this->sibling_ptr.get()    != nullptr and set_all_paths) this->sibling_ptr->set_path( folder_path, set_all_paths );
    if (this->child_dir_ptr.get()  != nullptr and set_all_paths) this->child_dir_ptr->set_path( folder_path, set_all_paths );
    if (this->child_file_ptr.get() != nullptr and set_all_paths) this->child_file_ptr->set_path( folder_path, set_all_paths );
}


void Folder::copy_to_another_archive( std::fstream& src, std::fstream& dst, uint64_t parent_location, uint64_t previous_sibling_location )
{
    if (!this->ptr_already_gotten) {    // if ptr_already_gotten, don't copy this
        src.seekg(this->location);
        uint64_t dst_location = dst.tellp();

        if (parent_location != 0)
        {
            if ( parent_ptr->child_dir_ptr.get() == this ) {
                assert(previous_sibling_location == 0);

                // update parent's knowledge of it's firstborn's location in file

                uint64_t backup_p = dst.tellp();

                dst.seekp( parent_location + 1 + parent_ptr->name_length + 8 ); // seekp( start of child_dir_location in the new archive )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( dst_location >> (i*8u)) & 0xFFu;

                dst.write((char*)buffer, 8);
                delete[] buffer;

                dst.seekp( backup_p );
            }
            else {
                assert(previous_sibling_location != 0);
                // update previous sibling's knowledge of it's next sibling's location
                Folder* previous_folder = parent_ptr->child_dir_ptr.get();
                while( previous_folder->sibling_ptr != nullptr )
                {
                    if (this != previous_folder->sibling_ptr.get())
                        previous_folder = previous_folder->sibling_ptr.get();
                    else
                        break;
                }

                uint64_t backup_p = dst.tellp();
                dst.seekp( previous_sibling_location + 1 + previous_folder->name_length + 16 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( dst_location >> (i*8u)) & 0xFFu;

                dst.write((char*)buffer, 8);
                delete[] buffer;

                dst.seekp( backup_p );
            }
        }

        dst.seekp(0, std::ios_base::end);

        uint32_t buffer_size = base_metadata_size+name_length;
        auto buffer = new uint8_t[buffer_size];
        uint32_t bi=0; //buffer index

        // (name length)
        buffer[bi] = this->name_length;
        bi++;

        // (file name)
        for (uint16_t i=0; i < name_length; i++)
            buffer[bi+i] = name[i];
        bi += name_length;

        // (parent location)
        for (uint8_t i = 0; i < 8; i++)
            buffer[bi + i] = (parent_location >> (i * 8u)) & 0xFFu;
        bi += 8;

        // (child_dir_ptr)
        for (uint8_t i = 0; i < 8; i++)
            buffer[bi + i] = 0;
        bi+=8;

        // (sibling_ptr) {
        for (uint8_t i = 0; i < 8; i++)
            buffer[bi + i] = 0;
        bi += 8;


        // (child_file_ptr)
        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = 0;
        bi += 8;

        assert( bi == buffer_size );

        dst.write((char*)buffer, buffer_size);
        delete[] buffer;

        if (sibling_ptr) sibling_ptr->copy_to_another_archive(src, dst, parent_location, dst_location);
        if (child_file_ptr) child_file_ptr->copy_to_another_archive(src, dst, dst_location, 0, 0);
        if (child_dir_ptr) child_dir_ptr->copy_to_another_archive(src, dst, dst_location, 0);
    }
    else
    {
        if (sibling_ptr) sibling_ptr->copy_to_another_archive(src, dst, parent_location, previous_sibling_location);
    }
}
