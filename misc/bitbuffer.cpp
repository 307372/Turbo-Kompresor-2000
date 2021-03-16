#include "bitbuffer.h"
#include <cmath>


TextWriteBitbuffer::TextWriteBitbuffer(std::string &output_string) {
    this->text = &output_string;
    bits_written=0;
    clear();
}


TextWriteBitbuffer::~TextWriteBitbuffer() {
    if ((bi != 0) or (bitcounter != 0)) flush();
}


void TextWriteBitbuffer::clear() {
    for (auto &i : buffer) i = 0;
    bi = 0;
    bitcounter = 0;
}


void TextWriteBitbuffer::flush() {
    if (bitcounter > 0) text->append( (char*)buffer, bi+1 );
    else text->append( (char*)buffer, bi );
    clear();
}



void TextWriteBitbuffer::add_bit_1() {
    buffer[bi] <<= 1u;
    ++buffer[bi];
    bitcounter++;
    bits_written++;

    if ( bitcounter == 8 ) {
        bi++;
        bitcounter = 0;
        if (bi >= 8*1024) flush();
    }
}


void TextWriteBitbuffer::add_bit_0() {
    buffer[bi] <<= 1u;
    bitcounter++;
    bits_written++;

    if ( bitcounter == 8 ) {
        bi++;
        bitcounter = 0;
        if (bi >= 8*1024) flush();
    }
}


uint64_t TextWriteBitbuffer::get_output_size() const {
    return bits_written;
}


TextReadBitbuffer::TextReadBitbuffer(uint8_t compressed_text[], uint64_t compressed_size, uint64_t starting_position) {
    this->text = compressed_text;
    this->byte_index = starting_position;
    this->data_left_b = starting_position + ceil((double)compressed_size/8.0);

    this->output_bit = false;

    this->bits_in_last_byte = compressed_size % 8;
    if (compressed_size > 8) this->meaningful_bits = 8;
    else this->meaningful_bits = compressed_size;
    this->bitcounter = 0;

}


bool TextReadBitbuffer::getbit() {
    output_bit = (text[byte_index] >> (meaningful_bits-bitcounter-1u)) & 1u; // probably not finished yet

    bitcounter++;
    if(bitcounter == 8) {
        bitcounter = 0;
        byte_index++;
        if (byte_index == data_left_b-1) {
            if (bits_in_last_byte != 0) meaningful_bits = bits_in_last_byte;
            else meaningful_bits = 8;
        }
    }
    return output_bit;
}

