#include "statistical_tools.h"
#include <fstream>
#include <iostream>
#include <map>
#include <climits>
#include <assert.h>
#include <cmath>
#include <iterator>
#include <chrono>

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

    auto start = std::chrono::high_resolution_clock::now();

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

    // for (auto i : r) std::cout << i << std::endl;

    // auto stop = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    // std::cout << std::endl << "duration: " << duration.count() << " microseconds\n";


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

    auto start = std::chrono::high_resolution_clock::now();

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






















