#include "archive_structures.h"



void file::interpret_flags(std::fstream &os, const std::string& path_to_destination, bool decode ) {

    std::bitset<16> bin_flags = this->flags_value;
    Compression comp;

    assert( bin_flags.any() );

    if ( bin_flags[0] ) {
        std::cout << "Bit 0/1\t\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[1] ) {
        std::cout << "Bit 1/2\t\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[2] ) {
        std::cout << "Bit 2/4\t\t\tis true - arithmetic coding with IID model for the whole file is used." << std::endl;
        Compression comp;
        if (!decode) { //
            //this->compressed_size = ac.encode_file_to_fstream( os, this->path, 8*1024 );

            if(comp.size == 0) {
                std::fstream input(this->path, std::ios::binary | std::ios::in);
                comp.load_text( input, this->uncompressed_size );
                input.close();
            }
            comp.BWT_make();
            comp.MTF_make();
            comp.RLE_make();
            comp.AC_make();
            comp.save_text(os);
            this->compressed_size = comp.size;

        } else { //decoding
            //ac.decode_to_file_from_fstream(os, path_to_destination + '/' + this->name, this->compressed_size, this->uncompressed_size);
            if(comp.size == 0) {
                //uint64_t backup_g = os.tellg();
                os.seekg(this->data_location);
                comp.load_text( os, this->compressed_size );
                //os.seekg(backup_g);
            }
            comp.AC_reverse();
            comp.RLE_reverse();
            comp.MTF_reverse();
            comp.BWT_reverse();
            std::cout << "Decoding path: " << path_to_destination << std::endl;
            std::fstream output(path_to_destination + "/" + this->name, std::ios::binary | std::ios::out);
            assert( output.is_open() );
            comp.save_text(output);
            output.close();
        }
    }
    if ( bin_flags[3] ) {
        std::cout << "Bit 3/8\t\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[4] ) {
        std::cout << "Bit 4/16\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[5] ) {
        std::cout << "Bit 5/32\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[6] ) {
        std::cout << "Bit 6/64\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[7] ) {
        std::cout << "Bit 7/128\t\tis true - files are copy-pasted using notcompression." << std::endl;
        notcompression nc;

        if (decode) {
            nc.decode(os, path_to_destination + "/" + this->name, this->compressed_size );
        } else {
            this->compressed_size = nc.encode( os, this->path );
        }
    }
    if ( bin_flags[8] ) {
        std::cout << "Bit 8/256\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[9] ) {
        std::cout << "Bit 9/512\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[10] ) {
        std::cout << "Bit 10/1024\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[11] ) {
        std::cout << "Bit 11/2048\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[12] ) {
        std::cout << "Bit 12/4096\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[13] ) {
        std::cout << "Bit 13/8192\t\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[14] ) {
        std::cout << "Bit 14/16384\tis true - " << std::endl;
        throw FlagReservedException();
    }
    if ( bin_flags[15] ) {
        std::cout << "Bit 15/32768\tis true - last 40 bytes of file data are SHA-1 hash." << std::endl;
        integrity_validation iv;
        if (!decode) { // if (encode)
            std::basic_string<char> sha_not_encoded = iv.get_SHA1(this->path);
            assert( sha_not_encoded.length() == 40 );
            os.write( sha_not_encoded.c_str(), sha_not_encoded.length() );
        } else { // decoding
            std::string sha_before_encoding = std::string( 40, ' ');
            os.read( &sha_before_encoding[0], 40 );
            std::string sha_after_decoding = iv.get_SHA1( path_to_destination + "/" + this->name );
            if ( sha_after_decoding == sha_before_encoding )
                std::cout << "File integrity confirmed.\n" << "before: " << sha_before_encoding << "\nafter:  " << sha_after_decoding << std::endl;
            else
                std::cout << "File integrity compromised!!!\n" << "before: " << sha_before_encoding << "\nafter:  " << sha_after_decoding << std::endl;
        }

    }
}

void file::recursive_print(std::ostream &os) const {
    os << *this << '\n';
    if (sibling_ptr) sibling_ptr->recursive_print( os );
}

std::ostream& operator<<(std::ostream &os, const file &f)
{
    os << "File named: \"" << f.name << "\", len(name) = " << f.name_length << '\n';
    os << "Has flags " << f.flags_value << '\n';
    os << "Header starts at byte " << f.location << ", with total size of " << file::base_metadata_size + f.name_length << " bytes\n";
    os << "Compressed data of this file starts at byte " << f.data_location << "\n";
    assert(f.parent_ptr);
    os << "Parent located at byte " << f.parent_ptr->location << ", ";//
    if (f.sibling_ptr) os << "Sibling located at byte " << f.sibling_ptr->location << '\n';
    else os << "there's no sibling\n";
    os << "With compressed size of " << f.compressed_size << " bits, and uncompressed size of " << f.uncompressed_size << " bytes." << std::endl;
    return os;
}

void file::parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<file> &shared_this ) {
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

    // Getting location of compressed data for this file from the archive
    os.read( (char*)buffer, 8 );
    this->data_location = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    // Getting size of compressed data of this file from the archive (in   B I T S)
    os.read( (char*)buffer, 8 );
    this->compressed_size = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    // Getting size of uncompressed data of this file from the archive
    os.read( (char*)buffer, 8 );
    this->uncompressed_size = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);

    if (sibling_location_pos != 0) {  // if there's another file in this dir, parse it too
        this->sibling_ptr = std::make_unique<file>();
        uint64_t backup_g = os.tellg();
        this->sibling_ptr->parse(os, sibling_location_pos, parent, sibling_ptr);
        os.seekg( backup_g );
    }

}

void file::write_to_archive( std::fstream &archive_file ) {

    if (!this->alreadySaved) {
        this->alreadySaved = true;
        location = archive_file.tellp();

        if (parent_ptr != nullptr) { // correcting current file's location in model, and in file
            if ( parent_ptr->child_file_ptr.get() == this ) {
               // std::cout << " parent_ptr->child_file_ptr.get() == this" << std::endl;
                uint64_t backup_p = archive_file.tellp();

                archive_file.seekp( parent_ptr->location + 1 + parent_ptr->name_length + 24 ); // seekp( start of child_file_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu; // Potentially fixed?

                archive_file.write((char*)buffer, 8);
                delete[] buffer;

                archive_file.seekp( backup_p );

            }
            else {
                file* file_ptr = parent_ptr->child_file_ptr.get(); // location of previous file in the same dir
                while( file_ptr->sibling_ptr != nullptr )
                {
                    if (this != file_ptr->sibling_ptr.get())
                        file_ptr = file_ptr->sibling_ptr.get();
                    else
                        break;
                }
                // std::cout << name << "\t" << location << std::endl;

                uint64_t backup_p = archive_file.tellp();
                archive_file.seekp( file_ptr->location + 1 + file_ptr->name_length + 8 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu; // Potentially fixed?

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

        if (sibling_ptr)
            for (uint8_t i=0; i < 8; i++)
                buffer[bi+i] = (sibling_ptr->location >> (i*8u)) & 0xFFu;
        else
            for (uint8_t i = 0; i < 8; i++)
                buffer[bi + i] = 0;
        bi+=8;

        for (uint8_t i=0; i < 2; i++)
            buffer[bi+i] = ((unsigned)flags_value >> (i*8u)) & 0xFFu;
        bi+=2;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (data_location >> (i*8u)) & 0xFFu;
        bi+=8;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (compressed_size >> (i * 8u)) & 0xFFu;
        bi+=8;

        for (uint8_t i=0; i < 8; i++)
            buffer[bi+i] = (uncompressed_size >> (i * 8u)) & 0xFFu;

        assert( bi+8 == buffer_size );
        archive_file.write((char*)buffer, buffer_size);
        delete[] buffer;


        auto backup_end_of_metadata = archive_file.tellp(); // position in file right after the end of metadata

        data_location = backup_end_of_metadata;

        interpret_flags( archive_file, "encoding has it's path in the file object", false );

        std::cout << "compressed size: " << compressed_size << std::endl;

        auto backup_p = archive_file.tellp();

        archive_file.seekp( backup_end_of_metadata );
        archive_file.seekp( -24, std::ios::cur );

        auto buffer2 = new uint8_t[16];

        for (uint8_t i=0; i < 8; i++)
            buffer2[i] = (data_location >> (i*8u)) & 0xFFu; // Potentially fixed?
        for (uint8_t i=0; i < 8; i++)
            buffer2[i+8] = (compressed_size >> (i*8u)) & 0xFFu;

        archive_file.write((char*)buffer2, 16);
        delete[] buffer2;

        archive_file.seekp( backup_p );
    }

    if (sibling_ptr) sibling_ptr->write_to_archive( archive_file );

}

void file::unpack( const std::string& path_to_destination, std::fstream &os, bool unpack_all )
{
    std::cout << "Unpacking file " << name << "..." << std::endl;

    uint64_t backup_g = os.tellg();
    os.seekg( this->data_location );

    //notcompression nc;
    // std::fstream f( path_to_destination + "/" + name, std::ios::out | std::ios::binary );

    interpret_flags( os, path_to_destination, true );
    //nc.decode( os, path_to_destination + "/" + name, compressed_size );

    os.seekg( backup_g );

    std::cout << "File " << name << " with compressed size of " << compressed_size << " unpacked\n" << std::endl;

    if (sibling_ptr != nullptr and unpack_all)
    {
        sibling_ptr->unpack( path_to_destination, os, unpack_all );
    }

}



// Folder methods below

folder::folder()= default;

folder::folder( std::unique_ptr<folder> &parent, std::string folder_name ) {
    name = std::move(folder_name);
    name_length = name.length();
    parent_ptr = parent.get();
}

folder::folder( folder* parent, std::string folder_name ) {
    name = std::move(folder_name);
    name_length = name.length();
    parent_ptr = parent;
}

void folder::recursive_print(std::ostream &os) const {
    os << *this << '\n';
    if (child_file_ptr) child_file_ptr->recursive_print( os );
    if (sibling_ptr) sibling_ptr->recursive_print( os );
    if (child_dir_ptr) child_dir_ptr->recursive_print( os );
}

std::ostream& operator<<(std::ostream& os, const folder& f){
    os << "Folder named: \"" << f.name << "\", len(name) = " << (uint32_t)f.name_length << '\n';
    os << "Header starts at byte " << f.location << ", with total size of " << folder::base_metadata_size + f.name_length << " bytes\n";
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

void folder::parse( std::fstream &os, uint64_t pos, folder* parent, std::unique_ptr<folder> &shared_this  )
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
        this->child_dir_ptr = std::make_unique<folder>();
        uint64_t backup_g = os.tellg();
        this->child_dir_ptr->parse(os, child_dir_pos_in_file, shared_this.get(), child_dir_ptr );
        os.seekg( backup_g );
    }


    os.read( (char*)buffer, 8 );
    uint64_t sibling_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);

    if (sibling_pos_in_file != 0) {
        this->sibling_ptr = std::make_unique<folder>();
        uint64_t backup_g = os.tellg();
        this->sibling_ptr->parse(os, sibling_pos_in_file, parent, sibling_ptr);
        os.seekg( backup_g );
    }


    os.read( (char*)buffer, 8 );
    uint64_t child_file_pos_in_file = ((uint64_t)buffer[0]) | ((uint64_t)buffer[1]<<8u) | ((uint64_t)buffer[2]<<16u) | ((uint64_t)buffer[3]<<24u) | ((uint64_t)buffer[4]<<32u) | ((uint64_t)buffer[5]<<40u) | ((uint64_t)buffer[6]<<48u) | ((uint64_t)buffer[7]<<56u);


    if (child_file_pos_in_file != 0) {
        this->child_file_ptr = std::make_unique<file>();
        uint64_t backup_g = os.tellg();
        this->child_file_ptr->parse(os, child_file_pos_in_file, shared_this.get(), child_file_ptr ); // Seemingly implemented
        os.seekg( backup_g );
    }
}

void folder::append_to_archive( std::fstream& archive_file ) {
    // archive_file.flush();
    archive_file.seekp(0, std::ios_base::end);
    this->write_to_archive( archive_file );
}

void file::append_to_archive( std::fstream& archive_file ) {
    archive_file.seekp(0, std::ios_base::end);
    this->write_to_archive( archive_file );
}

void folder::write_to_archive( std::fstream &archive_file ) {

    if (!this->alreadySaved) {
        this->alreadySaved = true;
        location = archive_file.tellp();

        if (parent_ptr != nullptr) { // correcting current dir's location in model, and in file
            if ( parent_ptr->child_dir_ptr.get() == this ) {
                // update parent's knowledge of it's firstborn's location in file
                std::cout << " parent_ptr->child_dir_ptr.get() == this" << std::endl;

                uint64_t backup_p = archive_file.tellp();

                archive_file.seekp( parent_ptr->location + 1 + parent_ptr->name_length + 8 ); // seekp( start of child_dir_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu; // Potentially fixed?

                archive_file.write((char*)buffer, 8);
                delete[] buffer;

                archive_file.seekp( backup_p );
            }
            else {
                folder* previous_folder = parent_ptr->child_dir_ptr.get();
                while( previous_folder->sibling_ptr != nullptr )
                {
                    if (this != previous_folder->sibling_ptr.get())
                        previous_folder = previous_folder->sibling_ptr.get();
                    else
                        break;
                }

                std::cout << name << "\t" << location << std::endl;

                uint64_t backup_p = archive_file.tellp();
                archive_file.seekp( previous_folder->location + 1 + previous_folder->name_length + 16 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( location >> (i*8u)) & 0xFFu; // Potentially fixed?

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
        // std::cout << "Folder " << name << " tellp() = " << archive_file.tellp() << std::endl;
        // std::cout << "child_folder_location " << child_file_location << std::endl;
        delete[] buffer;
    }

    if (sibling_ptr)
        sibling_ptr->write_to_archive( archive_file );

    if (child_file_ptr)
        child_file_ptr->write_to_archive( archive_file );

    if (child_dir_ptr)
        child_dir_ptr->write_to_archive( archive_file );

}

void folder::unpack( const std::filesystem::path& target_path, std::fstream &os, bool unpack_all ) const
{
    std::cout << "Creating folder " << name << "..." << std::endl;
    std::filesystem::path path_with_this_folder( target_path.string() + '/' + name );
    if ( !std::filesystem::exists(path_with_this_folder) )
        std::filesystem::create_directories( path_with_this_folder );

    std::cout << "Folder " << name << " created\n" << std::endl;

    if (unpack_all) {
        if( sibling_ptr != nullptr )
            sibling_ptr->unpack(target_path, os, unpack_all);
        if( child_dir_ptr != nullptr )
            child_dir_ptr->unpack( path_with_this_folder, os, unpack_all);
        if( child_file_ptr != nullptr )
            child_file_ptr->unpack( path_with_this_folder, os, unpack_all);
    }

}
