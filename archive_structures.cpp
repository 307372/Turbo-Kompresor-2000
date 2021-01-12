#include "archive_structures.h"

uint16_t calculate_progress( float current, float whole ) { return roundf(current*100 / whole); }

void processing_worker( const int task, Compression* comp, uint16_t flags, bool& aborting_var, bool* is_finished, uint16_t* progress_ptr = nullptr )
{
    std::bitset<16> bin_flags = flags;
    uint16_t progress_counter = 0;
    uint16_t whole = bin_flags.count();
    // std::cout << "worker " << comp->part_id << " size: " << comp->size << '\n' << std::flush;
    if (task == Compression::compress) {
        if (bin_flags[0] and !aborting_var) {
            std::cout << "BWT encoding..." << std::endl;
            comp->BWT_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);

        }

        if (bin_flags[1] and !aborting_var) {
            std::cout << "MTF encoding..." << std::endl;
            comp->MTF_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if (bin_flags[2] and !aborting_var) {
            std::cout << "RLE encoding..." << std::endl;
            comp->RLE_makeV2();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if (bin_flags[3] and !aborting_var) {
            std::cout << "AC encoding..." << std::endl;
            comp->AC_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if (bin_flags[4] and !aborting_var) {
            std::cout << "AC2 encoding..." << std::endl;
            comp->AC2_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }
    }
    else if (task == Compression::decompress)
    {
        if ( bin_flags[4] and !aborting_var) {
            std::cout << "AC2 decoding..." << std::endl;
            comp->AC2_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[3] and !aborting_var) {
            std::cout << "AC decoding..." << std::endl;
            comp->AC_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[2] and !aborting_var) {
            std::cout << "RLE decoding..." << std::endl;
            comp->RLE_reverseV2();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[1] and !aborting_var) {
            std::cout << "MTF decoding..." << std::endl;
            comp->MTF_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[0] and !aborting_var ) {
            std::cout << "BWT decoding..." << std::endl;
            comp->BWT_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }
    }
    *is_finished = true;
}

void processing_scribe( const int task, std::fstream& output, std::vector<Compression*>& comp_v, bool worker_finished[],
                        std::vector<std::thread>& workers, uint32_t block_count, uint64_t* compressed_size,
                        std::string& checksum, bool& checksum_done, uint64_t original_size, bool& aborting_var )
{
    assert( output.is_open() );
    uint32_t next_to_write = 0;  // index of last written block of data in comp_v
    *compressed_size = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while ( next_to_write != block_count )
    {
        if (worker_finished[next_to_write]) {
            if (task == Compression::compress) {
                std::stringstream block_metadata;
                block_metadata.write((char *) &next_to_write, sizeof(next_to_write)); // part number
                block_metadata.write((char *) &comp_v[next_to_write]->size,
                                     sizeof(comp_v[next_to_write]->size));
                output << block_metadata.rdbuf();
                *compressed_size += comp_v[next_to_write]->size;
            }

            comp_v[next_to_write]->save_text(output);
            delete comp_v[next_to_write];
            comp_v[next_to_write] = nullptr;
            next_to_write++;
        }
        else std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (task == Compression::compress) {
        while (!checksum_done) std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (checksum.length() != 0) output.write(checksum.c_str(), checksum.length());
    }
    else if (task == Compression::decompress)
    {
        while (!checksum_done) std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (checksum.length() != 0)
        {

            integrity_validation iv;
            std::string new_checksum;
            if ( checksum.length() == 10 )  // CRC-32
            {
                new_checksum = iv.get_CRC32_from_stream(output, aborting_var);
                std::cout << "CRC-32:\n";
            }
            else if ( checksum.length() == 40 ) // SHA-1
            {
                new_checksum = iv.get_SHA1_from_stream(output, original_size, aborting_var);
                std::cout << "SHA-1:\n";
            }
            else std::cout << "Weird checksum..." << std::endl;
            std::cout << "Checksums:\nOld: " << checksum << "\nNew: " << new_checksum << '\n';
            if (new_checksum == checksum) std::cout << "Decompression successful!" << std::endl;
            else std::cout << "Decompression failed!" << std::endl;
        }

    }
    if (output.is_open() and task == Compression::decompress) output.close();
}

void processing_foreman( std::fstream &archive_stream, const std::string& target_path, const int task, uint16_t flags,
                         uint64_t original_size, uint64_t* compressed_size, bool& aborting_var, bool validate_integrity, uint16_t* progress_ptr )
{
    // 1. delegates work to each thread
    // 2. calculates checksum and sends it to scribe, after every block of data is processed
    std::bitset<16> bin_flags = flags;
    uint32_t block_size = 1 << 24;  // 2^24 Bytes = 16 MiB, default block size

    if ( bin_flags[9] ) block_size >>= 1;
    if ( bin_flags[10] ) block_size >>= 2;
    if ( bin_flags[11] ) block_size >>= 4;
    if ( bin_flags[12] ) block_size >>= 8;
    // std::cout << "Block size: " << block_size << std::endl;


    uint32_t block_count = ceill((long double)original_size / block_size);
    if (block_count == 1) block_size = original_size;

    // preparing vector of empty Compression objects for threads
    std::vector<Compression*> comp_v;
    for (uint32_t i=0; i < block_count; ++i) comp_v.emplace_back(new Compression(aborting_var));


    uint32_t worker_count = std::thread::hardware_concurrency();
    // "if value is not well defined or not computable, (std::thread::hardware_concurrency) returns 0" ~cppreference.com
    if (worker_count == 0) worker_count = 2;
    if (worker_count > block_count) worker_count = block_count;


    // std::fstream input(src_path, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream target_stream;
    if (task == Compression::compress) target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
    else if (task == Compression::decompress)
    {
        target_stream.open(target_path, std::ios::binary | std::ios::out);  // making sure target file exists
        target_stream.close();
        target_stream.open(target_path, std::ios::binary | std::ios::in | std::ios::out);
    }
    assert( archive_stream.is_open() );
    assert( target_stream.is_open() );


    bool* task_finished_arr = new bool[block_count];
    bool* task_started_arr  = new bool[block_count];
    for (uint32_t i=0; i < block_count; ++i) {
        task_finished_arr[i] = false;
        task_started_arr[i] = false;
    }
    std::vector<uint16_t> progress_counters(block_count, 0);

    std::vector<std::thread> workers;
    std::string checksum;
    bool checksum_done = false;
    uint32_t lowest_free_work_ind = 0;

    // filling compression objects, and starting worker threads to process them
    for ( uint32_t i=0; i < worker_count; ++i )
    {
        if (aborting_var) break;
        if (task == Compression::compress) {
            comp_v[i]->load_part(target_stream, original_size, i, block_size);
            comp_v[i]->part_id = i;
            if (compressed_size != nullptr) *compressed_size = 0;
        }
        else if (task == Compression::decompress) {
            archive_stream.read((char*)&comp_v[i]->part_id, sizeof(comp_v[i]->part_id));
            archive_stream.read((char*)&comp_v[i]->size, sizeof(comp_v[i]->size));
            comp_v[i]->load_text(archive_stream, comp_v[i]->size);
        }
        workers.emplace_back(&processing_worker, task, comp_v[i], flags, std::ref(aborting_var),
                             &task_finished_arr[i], &progress_counters[i]);
        task_started_arr[i] = true;
        lowest_free_work_ind++;
    }
    if (aborting_var)
    {
        for (auto& th: workers) th.join();
        delete[] task_finished_arr;
        delete[] task_started_arr;
        for (auto & comp : comp_v) delete comp;

        return;
    }

    std::thread scribe;

    if (task == Compression::compress)
        scribe = std::thread( &processing_scribe, task, std::ref(archive_stream), std::ref(comp_v),
                              task_finished_arr, std::ref(workers), block_count, compressed_size,
                              std::ref(checksum), std::ref(checksum_done), original_size, std::ref(aborting_var) );
    else if (task == Compression::decompress)
        scribe = std::thread( &processing_scribe, task, std::ref(target_stream), std::ref(comp_v), task_finished_arr,
                              std::ref(workers), block_count, compressed_size, std::ref(checksum), std::ref(checksum_done),
                              original_size, std::ref(aborting_var) );

    while (lowest_free_work_ind != block_count and !aborting_var) {

        for (uint32_t i=0; i < workers.size() and !aborting_var; ++i) {
            if (workers[i].joinable()) {
                if (workers[i].joinable()) workers[i].join();
                if (aborting_var) break;

                if (lowest_free_work_ind != block_count) {
                    if (task == Compression::compress) {
                        comp_v[lowest_free_work_ind]->load_part(target_stream, original_size, lowest_free_work_ind, block_size);
                        comp_v[lowest_free_work_ind]->part_id = lowest_free_work_ind;
                    }
                    else if (task == Compression::decompress) {
                        archive_stream.read((char*)&comp_v[lowest_free_work_ind]->part_id, sizeof(comp_v[lowest_free_work_ind]->part_id));
                        archive_stream.read((char*)&comp_v[lowest_free_work_ind]->size, sizeof(comp_v[lowest_free_work_ind]->size));
                        comp_v[lowest_free_work_ind]->load_text(archive_stream, comp_v[lowest_free_work_ind]->size);
                    }
                    else std::cout << "It's neither compression nor decompression?" << std::endl;
                    workers.emplace_back(&processing_worker,
                                         task,
                                         comp_v[lowest_free_work_ind],
                                         flags,
                                         std::ref(aborting_var),
                                         &task_finished_arr[lowest_free_work_ind],
                                         &progress_counters[lowest_free_work_ind]);

                    lowest_free_work_ind++;
                }
            }
        }
    }

    if (aborting_var)
    {
        for (auto& th: workers) th.join();
        if (scribe.joinable()) scribe.join();
        delete[] task_finished_arr;
        delete[] task_started_arr;
        for (auto & comp : comp_v) delete comp;

        return;
    }

    if (task == Compression::compress) {
        // since we're done with giving workers work, we can calculate checksum, which scribe thread will append to file

        if (bin_flags[15])  // SHA-1
        {
            integrity_validation iv;
            checksum = iv.get_SHA1_from_file(target_path, aborting_var);
        }
        if (bin_flags[14])  // CRC-32
        {
            integrity_validation iv;
            checksum = iv.get_CRC32_from_file(target_path, aborting_var);
        }
        checksum_done = true;
    }
    else if (task == Compression::decompress)
    {
        if (bin_flags[15])  // SHA-1
        {
            checksum = std::string(40, 0x00);
            archive_stream.read((char*)checksum.data(), checksum.length());
            // std::cout << "Read SHA-1" << checksum << std::endl;
        }
        if (bin_flags[14])  // CRC-32
        {
            checksum = std::string(10, 0x00);
            archive_stream.read((char*)checksum.data(), checksum.length());
        }
        checksum_done = true;
    }

    for (auto& th : workers) if (th.joinable()) th.join();
    if (scribe.joinable()) scribe.join();

    delete[] task_finished_arr;
    delete[] task_started_arr;
    for (auto & comp : comp_v) delete comp;
}



void file::interpret_flags(std::fstream &archive_stream, const std::string& path_to_destination, bool encode, bool& aborting_var, bool validate_integrity, uint16_t* progress_ptr ) {

    if (encode == true)
    {
        processing_foreman(archive_stream, this->path, Compression::compress, flags_value, uncompressed_size,
                           &this->compressed_size, aborting_var, validate_integrity, progress_ptr );
    }
    else
    {
        archive_stream.seekg(this->data_location);
        processing_foreman(archive_stream, path_to_destination + '/' + this->name, Compression::decompress, flags_value,
                           uncompressed_size, &this->compressed_size, aborting_var, validate_integrity, progress_ptr );
    }

    /*
    std::bitset<16> bin_flags = this->flags_value;
    std::cout << "flags:\n" << bin_flags.to_string() << std::endl;
    Compression comp( aborting_var );

    assert( bin_flags.any() );
    uint16_t progress_counter = 0;
    uint16_t whole = bin_flags.count();
    std::cout << "Progress ptr = " << progress_ptr << std::endl;

    if (progress_ptr != nullptr) (*progress_ptr) = 0;
    // loading data for processing into Compression object, and then processing it according to flags


    if (encode) {
        std::fstream input(this->path, std::ios::binary | std::ios::in);
        comp.load_text( input, this->uncompressed_size );

        input.close();

        std::basic_string<char> CRC32_before;

        if ( bin_flags[14] ) {
            integrity_validation iv;
            CRC32_before = iv.get_CRC32_from_text(comp.text, comp.size, aborting_var);
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);

        }

        if ( bin_flags[0] ) {
            std::cout << "BWT encoding..." << std::endl;
            comp.BWT_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);

        }

        if ( bin_flags[1] ) {
            std::cout << "MTF encoding..." << std::endl;
            comp.MTF_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[2] ) {
            std::cout << "RLE encoding..." << std::endl;
            comp.RLE_makeV2();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[3] ) {
            std::cout << "AC encoding..." << std::endl;
            comp.AC_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[4] ) {
            std::cout << "AC2 encoding..." << std::endl;
            comp.AC2_make();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
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
            std::cout << "Bit 7/128\t\tis true - " << std::endl;

            throw FlagReservedException();
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

        comp.save_text(archive_stream);
        this->compressed_size = comp.size;

        if ( bin_flags[14] ) {
            std::cout << "Appending CRC-32..." << std::endl;
            if (!aborting_var) {
                assert( CRC32_before.length() == 10 );
                archive_stream.write(CRC32_before.c_str(), CRC32_before.length() );
            }
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);

        }


        if ( bin_flags[15] ) {
            std::cout << "Appending SHA-1..." << std::endl;
            integrity_validation iv;
            std::basic_string<char> sha_not_encoded = iv.get_SHA1_from_file(this->path, aborting_var);
            if (!aborting_var) {
                assert( sha_not_encoded.length() == 40 );
                archive_stream.write(sha_not_encoded.c_str(), sha_not_encoded.length() );
            }
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        std::cout << "Encoding finished." << std::endl;

    }
    else {  // decompression
        archive_stream.seekg(this->data_location);
        comp.load_text(archive_stream, this->compressed_size );



        if ( bin_flags[13] ) {
            std::cout << "Bit 13/8192\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[12] ) {
            std::cout << "Bit 12/4096\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[11] ) {
            std::cout << "Bit 11/2048\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[10] ) {
            std::cout << "Bit 10/1024\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[9] ) {
            std::cout << "Bit 9/512\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[8] ) {
            std::cout << "Bit 8/256\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[7] ) {
            std::cout << "Bit 7/128\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[6] ) {
            std::cout << "Bit 6/64\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[5] ) {
            std::cout << "Bit 5/32\t\tis true - " << std::endl;
            throw FlagReservedException();
        }

        if ( bin_flags[4] ) {
            std::cout << "AC2 decoding..." << std::endl;
            comp.AC2_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[3] ) {
            std::cout << "AC decoding..." << std::endl;
            comp.AC_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[2] ) {
            std::cout << "RLE decoding..." << std::endl;
            comp.RLE_reverseV2();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[1] ) {
            std::cout << "MTF decoding..." << std::endl;
            comp.MTF_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }

        if ( bin_flags[0] ) {
            std::cout << "BWT decoding..." << std::endl;
            comp.BWT_reverse();
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }


        std::cout << "Decoding path: " << path_to_destination << std::endl;
        std::fstream output(path_to_destination + "/" + this->name, std::ios::binary | std::ios::out);
        assert( output.is_open() );
        comp.save_text(output);
        output.close();
        std::cout << "Decoding finished." << std::endl;

        if ( bin_flags[14] and validate_integrity ) {
            std::cout << "Comparing CRC-32..." << std::endl;
            integrity_validation iv;

            std::string CRC32_before = std::string( 10, ' ');
            archive_stream.read(&CRC32_before[0], 10 );

            std::string CRC32_after = iv.get_CRC32_from_text(comp.text, comp.size, aborting_var);
            if ( !aborting_var ) {
                if ( CRC32_before == CRC32_after )
                    std::cout << "File integrity confirmed.\n" << "before: " << CRC32_before << "\nafter:  " << CRC32_after << std::endl;
                else
                    std::cout << "File integrity compromised!!!\n" << "before: " << CRC32_before << "\nafter:  " << CRC32_after << std::endl;
            }
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);

        }

        if ( bin_flags[15] and validate_integrity ) {
            std::cout << "Comparing SHA-1..." << std::endl;
            integrity_validation iv;

            std::string sha_before_encoding = std::string( 40, ' ');
            archive_stream.read(&sha_before_encoding[0], 40 );
            std::string sha_after_decoding = iv.get_SHA1_from_file( path_to_destination + "/" + this->name, aborting_var );
            if ( !aborting_var ) {
                if ( sha_after_decoding == sha_before_encoding )
                    std::cout << "File integrity confirmed.\n" << "before: " << sha_before_encoding << "\nafter:  " << sha_after_decoding << std::endl;
                else
                    std::cout << "File integrity compromised!!!\n" << "before: " << sha_before_encoding << "\nafter:  " << sha_after_decoding << std::endl;
            }
            progress_counter++;
            if (progress_ptr != nullptr) *progress_ptr = calculate_progress(progress_counter, whole);
        }
    }
*/
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
        assert(sibling_location_pos < UINT32_MAX);  // temporary
        this->sibling_ptr = std::make_unique<file>();
        uint64_t backup_g = os.tellg();
        this->sibling_ptr->parse(os, sibling_location_pos, parent, sibling_ptr);
        os.seekg( backup_g );
    }

}

void file::write_to_archive( std::fstream &archive_file, bool& aborting_var, bool write_siblings, uint16_t* progress_var ) {
    std::cout << "name of this file is: " << this->name << std::endl;
    if (!this->alreadySaved and !aborting_var) {
        this->alreadySaved = true;
        location = archive_file.tellp();

        if (parent_ptr != nullptr) {
            // correcting current file's location in model, and in the file
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
                file* file_ptr = parent_ptr->child_file_ptr.get(); // location of previous file in the same dir
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
            buffer[bi+i] = (uncompressed_size >> (i * 8u)) & 0xFFu;
        bi+=8;

        assert( bi == buffer_size );
        archive_file.write((char*)buffer, buffer_size);
        delete[] buffer;


        auto backup_end_of_metadata = archive_file.tellp(); // position in file right after the end of metadata

        data_location = backup_end_of_metadata;

        // encoding
        interpret_flags( archive_file, "encoding has it's path in the file object", true, aborting_var, true, progress_var );

        std::cout << "compressed size: " << compressed_size << std::endl;

        auto backup_p = archive_file.tellp();

        archive_file.seekp( data_location - 24 );
        std::cout << "current put marker: " << archive_file.tellp() << std::endl;
        std::cout << "end of metadata   : " << backup_end_of_metadata << std::endl;

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

}

void file::unpack( const std::string& path_to_destination, std::fstream &os, bool& aborting_var, bool unpack_all, bool validate_integrity, uint16_t* progress_var )
{
    std::cout << "Unpacking file " << name << "..." << std::endl;

    uint64_t backup_g = os.tellg();
    os.seekg( this->data_location );

    interpret_flags( os, path_to_destination, false, aborting_var, validate_integrity, progress_var );

    os.seekg( backup_g );

    std::cout << "File " << name << " with compressed size of " << compressed_size << " unpacked\n" << std::endl;

    if (sibling_ptr != nullptr and unpack_all)
    {
        sibling_ptr->unpack( path_to_destination, os, aborting_var, unpack_all, validate_integrity, progress_var );
    }

}

std::string file::get_compressed_filesize_str(bool scaled) {
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

std::string file::get_uncompressed_filesize_str(bool scaled) {
    std::string units[5] = {"B","KB","MB","GB","TB"};

    float filesize = this->uncompressed_size;
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

void folder::append_to_archive( std::fstream& archive_file, bool& aborting_var ) {
    // archive_file.flush();
    archive_file.seekp(0, std::ios_base::end);
    this->write_to_archive( archive_file, aborting_var );
}

void file::append_to_archive( std::fstream& archive_file, bool& aborting_var, bool write_siblings, uint16_t* progress_var ) {
    archive_file.seekp(0, std::ios_base::end);
    this->write_to_archive( archive_file, aborting_var, write_siblings, progress_var );
}

void folder::write_to_archive( std::fstream &archive_file, bool& aborting_var ) {

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
        sibling_ptr->write_to_archive( archive_file, aborting_var );

    if (child_file_ptr)
        child_file_ptr->write_to_archive( archive_file, aborting_var );

    if (child_dir_ptr)
        child_dir_ptr->write_to_archive( archive_file, aborting_var );

}

void folder::unpack( const std::filesystem::path& target_path, std::fstream &os, bool& aborting_var, bool unpack_all ) const
{
    std::string temp_name;
    if (this->parent_ptr == nullptr) temp_name = std::filesystem::path(this->name).stem();
    else temp_name = this->name;
    std::cout << "Creating folder " << temp_name << "..." << std::endl;
    std::filesystem::path path_with_this_folder( target_path.string() + '/' + temp_name );
    if ( !std::filesystem::exists(path_with_this_folder) )
        std::filesystem::create_directories( path_with_this_folder );

    std::cout << "Folder " << temp_name << " created\n" << std::endl;

    if (unpack_all) {
        if( sibling_ptr != nullptr )
            sibling_ptr->unpack(target_path, os, aborting_var, unpack_all);
        if( child_dir_ptr != nullptr )
            child_dir_ptr->unpack( path_with_this_folder, os, aborting_var, unpack_all);
        if( child_file_ptr != nullptr )
            child_file_ptr->unpack( path_with_this_folder, os, aborting_var, unpack_all);
    }

}

void folder::get_ptrs( std::vector<folder*>& folders, std::vector<file*>& files ) {
    if ( !this->ptr_already_gotten ) {
        folders.emplace_back( this );
        this->ptr_already_gotten = true;
    }

    if (this->child_dir_ptr.get() != nullptr) this->child_dir_ptr->get_ptrs( folders, files );
    if (this->child_file_ptr.get() != nullptr) this->child_file_ptr->get_ptrs( files, true );

}

void file::get_ptrs( std::vector<file*>& files, bool get_siblings_too ) {
    if ( !this->ptr_already_gotten ) {
        files.emplace_back( this );
        this->ptr_already_gotten = true;
    }

    if ( this->sibling_ptr.get() != nullptr and get_siblings_too ) this->sibling_ptr->get_ptrs( files, get_siblings_too );
}


void folder::set_path( std::filesystem::path extraction_path, bool set_all_paths ) {
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

    if (this->child_dir_ptr.get() != nullptr and set_all_paths) this->child_dir_ptr->set_path( folder_path, set_all_paths );
    if (this->child_file_ptr.get() != nullptr and set_all_paths) this->child_file_ptr->set_path( folder_path, set_all_paths );
}

void file::set_path( std::filesystem::path extraction_path, bool set_all_paths ) {
    if ( this->ptr_already_gotten ) this->path = extraction_path;

    if ( this->sibling_ptr.get() != nullptr and set_all_paths ) this->sibling_ptr->set_path( extraction_path, set_all_paths );
}

void folder::copy_to_another_file( std::fstream& src, std::fstream& dst, uint64_t parent_location, uint64_t previous_sibling_location )
{
    if (!this->ptr_already_gotten) {    // if ptr_already_gotten, don't copy this
        src.seekg(this->location);
        uint64_t dst_location = dst.tellp();

        if (parent_location != 0)
        {
            if ( parent_ptr->child_dir_ptr.get() == this ) {
                assert(previous_sibling_location == 0);

                // update parent's knowledge of it's firstborn's location in file
                std::cout << " parent_ptr->child_dir_ptr.get() == this" << std::endl;

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
                folder* previous_folder = parent_ptr->child_dir_ptr.get();
                while( previous_folder->sibling_ptr != nullptr )
                {
                    if (this != previous_folder->sibling_ptr.get())
                        previous_folder = previous_folder->sibling_ptr.get();
                    else
                        break;
                }

                std::cout << name << "\t" << location << std::endl;

                uint64_t backup_p = dst.tellp();
                dst.seekp( previous_sibling_location + 1 + previous_folder->name_length + 16 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( dst_location >> (i*8u)) & 0xFFu; // Potentially fixed?

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
        // std::cout << "Folder " << name << " tellp() = " << archive_file.tellp() << std::endl;
        // std::cout << "child_folder_location " << child_file_location << std::endl;
        delete[] buffer;

        if (sibling_ptr) sibling_ptr->copy_to_another_file(src, dst, parent_location, dst_location);
        if (child_file_ptr) child_file_ptr->copy_to_another_file(src, dst, dst_location, 0);
        if (child_dir_ptr) child_dir_ptr->copy_to_another_file(src, dst, dst_location, 0);
    }
    else
    {
        assert(previous_sibling_location < UINT32_MAX and parent_location < UINT32_MAX);  // temporary
        if (sibling_ptr) sibling_ptr->copy_to_another_file(src, dst, parent_location, previous_sibling_location);
    }


}

void file::copy_to_another_file( std::fstream& src, std::fstream& dst, uint64_t parent_location, uint64_t previous_sibling_location )
{
    if (!this->ptr_already_gotten) {    // if ptr_already_gotten, don't copy this

        dst.seekp(0, std::ios_base::end);
        uint64_t dst_location = dst.tellp();

        src.seekg(this->location);

        if (parent_ptr != nullptr) {
            assert(parent_location != 0);
            // correcting current file's location in model, and in the file
            if ( parent_ptr->child_file_ptr.get() == this or previous_sibling_location == 0 ) {


                dst.seekp( parent_location + 1 + parent_ptr->name_length + 24 ); // seekp( start of child_file_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( dst_location >> (i*8u)) & 0xFFu;

                dst.write((char*)buffer, 8);
                delete[] buffer;



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


                dst.seekp( previous_sibling_location + 1 + file_ptr->name_length + 8 ); // seekp( start of sibling_location in archive file )
                auto buffer = new uint8_t[8];

                for (uint8_t i=0; i < 8; i++)
                    buffer[i] = ( dst_location >> (i*8u)) & 0xFFu;

                dst.write((char*)buffer, 8);
                delete[] buffer;
            }
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
            buffer[bi+i] = (uncompressed_size >> (i * 8u)) & 0xFFu;
        bi+=8;


        assert( bi == buffer_size );
        dst.write((char*)buffer, buffer_size);
        delete[] buffer;


        uint64_t backup_end_of_metadata = dst.tellp(); // position in file right after the end of metadata
        std::cout << "data location calculation: " << dst_location - this->location + this->data_location << std::endl;
        assert( backup_end_of_metadata == dst_location - this->location + this->data_location );

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


        if (sibling_ptr) sibling_ptr->copy_to_another_file(src, dst, parent_location, dst_location);
    }
    else if (sibling_ptr){
        assert(previous_sibling_location < UINT32_MAX);
        sibling_ptr->copy_to_another_file(src, dst, parent_location, previous_sibling_location);

    }
}










