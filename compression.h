#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <fstream>
#include <random>

class Compression {
public:
    bool* aborting_var;
    uint8_t* text;
    uint32_t size;
    uint32_t part_id=0;

    Compression( bool& aborting_variable );
    ~Compression();

    void load_text( std::fstream &input, uint64_t text_size );
    void load_part( std::fstream &input, uint64_t text_size, uint32_t part_num, uint32_t block_size );
    void save_text( std::fstream &output );

    void BWT_make();    // Burrows-Wheeler transform (DC3)
    void BWT_reverse();

    void BWT_make2();   // Burrows-Wheeler transform (divsufsort)
    void BWT_reverse2();

    void MTF_make();    // move-to-front (savage)
    void MTF_reverse();

    void RLE_make();    // run-length encoding (interlaced)
    void RLE_reverse();

    void RLE_makeV2();  // run-length encoding (separated)
    void RLE_reverseV2();

    void AC_make();     // arithmetic coding (memoryless model)
    void AC_reverse();

    void AC2_make();    // arithmetic coding (first-order Markov model)
    void AC2_reverse();

    void rANS_make();   // asymmetric numeral systems (range variant)
    void rANS_reverse();
};

#endif //COMPRESSION_DEV_COMPRESSION_H
