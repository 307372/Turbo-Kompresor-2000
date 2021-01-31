#include "statistical_tools.h"
#include <fstream>
#include <iostream>
#include <map>
#include <climits>
#include <assert.h>
#include <cmath>
#include <iterator>
#include <chrono>
#include <vector>
#include <filesystem>
#include <cassert>
#include <numeric>


statistical_tools::statistical_tools( std::string& absolute_path_to_file )
{
    for (auto &i : this->r) i = 0;
    this->alphabet = "";
    this->target_file.open( absolute_path_to_file );
    this->file_size = std::filesystem::file_size( absolute_path_to_file );
    this->max = UINT_MAX;
    this->entropy = 0;
}


statistical_tools::statistical_tools() {    // fileless version
    for (auto &i : this->r) i = 0;
    this->alphabet = "";
    this->file_size = 0;
    this->max = UINT_MAX;
    this->entropy = 0;
}


statistical_tools::~statistical_tools()
{
    if (this->target_file.is_open())
        this->target_file.close();
}


void statistical_tools::iid_model_chunksV2(size_t buffer_size) {
    //Produces statistical data compatible with my arithmetic coding implementation.
    //It reads the file in chunks until EOF.

    assert( this->target_file.is_open() );

    uint8_t buffer[buffer_size]; //reads only buffer_size bytes at a time

    while (!this->target_file.eof()) {
        this->target_file.read((char *) buffer, buffer_size);
        std::streamsize dataSize = this->target_file.gcount();
        for (uint64_t i = 0; i < dataSize; i++) this->r[buffer[i]]++;
    }


    for (uint32_t i = 0; i <= UCHAR_MAX; i++) {
        r[i] *= this->max;
        r[i] /= this->file_size;
    }

    uint64_t rsum2 = std::accumulate(r, r + 256, 0ull);
    if (rsum2 > this->max) {
        *std::max_element(std::begin(r), std::end(r)) -= rsum2 - this->max;
        rsum2 = this->max;
    } else if (rsum2 < this->max) {
        *std::max_element(std::begin(r), std::end(r)) += this->max - rsum2;
        rsum2 = this->max;
    }

    assert(rsum2 == this->max);
}


void statistical_tools::get_entropy()
{
    this->entropy = 0;
    for (auto& ri : this->r)
    {
        double p = ri/((double)(this->max));
        this->entropy -= p*log2(p);
    }

}


void statistical_tools::iid_model_from_text( uint8_t text[], uint32_t text_size ) {

    for (uint32_t i=0; i < text_size; ++i) {
        this->r[text[i]]++;
    }

    for (uint32_t i = 0; i <= UCHAR_MAX; i++) {
        r[i] *= this->max;
        r[i] /= text_size;
    }

    uint64_t rsum2 = std::accumulate(r, r + 256, 0ull);
    if (rsum2 > this->max) {
        *std::max_element(std::begin(r), std::end(r)) -= rsum2 - this->max;
        rsum2 = this->max;
    } else if (rsum2 < this->max) {
        *std::max_element(std::begin(r), std::end(r)) += this->max - rsum2;
        rsum2 = this->max;
    }

    assert(rsum2 == this->max);

}


void statistical_tools::model_from_text_1back( uint8_t text[], uint32_t text_size ) {


    for (auto & i : rr) for (auto & j : i) j = 0;   // zeroing rr
    for (auto & i : r)  i = 0;   // zeroing r

    uint8_t previous_char = text[0];
    for (uint32_t i=1; i < text_size; ++i) {
        rr[previous_char][text[i]]++;
        r[previous_char]++;
        previous_char = text[i];
    }

    // scaling every tab, so that sum of their elements = this->max
    for (uint16_t i=0; i < 256; ++i) for (uint16_t j=0; j < 256; ++j) {
        bool less_than_one = false;
        if ( (long double)rr[i][j]*(long double)this->max/(long double)r[i] < 1 and rr[i][j] != 0 ) less_than_one = true;
        rr[i][j]=(uint64_t)(roundl((long double)rr[i][j]*(long double)this->max/(long double)r[i]));
        if (less_than_one) assert( rr[i][j] != 0 );

    }

    for (auto & current_r : rr) {
        int64_t diff = (int64_t)UINT32_MAX - std::accumulate(std::begin(current_r), std::end(current_r), 0ull);
        if (diff == UINT32_MAX or diff == 0) continue;  // if vector is empty or sum of its element is exacly what we want, continue
        else if (diff < 0) {

            while (diff != 0) {
                for (uint32_t ch=0; ch < 256 and diff != 0; ++ch) {
                    if ( current_r[ch] > std::abs(diff) and current_r[ch] != 0 )
                    {
                        current_r[ch]--;
                        diff++;
                    }
                }
            }
            diff = (int64_t)UINT32_MAX - std::accumulate(std::begin(current_r), std::end(current_r), 0ull);
        }
        else if (diff > 0)
        {
            while (diff != 0)
                for (uint32_t ch=255; ch >=0  and diff != 0; --ch)
                    if ( current_r[ch] > std::abs(diff) and current_r[ch] != 0 )
                    {
                        current_r[ch]++;
                        diff--;
                    }
            diff = (int64_t)UINT32_MAX - std::accumulate(std::begin(current_r), std::end(current_r), 0ull);
        }

        assert(diff == 0 or diff == UINT32_MAX);
    }
}


void statistical_tools::count_symbols_from_text_0back( uint8_t text[], uint32_t text_size )
{
    for (auto& r_n : this->r) r_n = 0;
    for (uint32_t i=0; i < text_size; ++i) this->r[text[i]]++;
}


void statistical_tools::count_symbols_from_text_1back(uint8_t text[], uint32_t text_size) {
    for (auto& r_n : this->r) r_n = 0;
    for (auto& rr_n : this->rr) for (auto& r_n : rr_n) r_n = 0;

    uint8_t previous = text[0];
    for (uint32_t i=1; i < text_size; ++i) {
        this->r[previous]++;
        this->rr[previous][text[i]]++;
        previous = text[0];
    }
}
