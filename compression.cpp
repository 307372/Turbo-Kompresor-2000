#include "compression.h"

#include <iostream>
#include <list>
#include <climits>
#include <cmath>
#include <cassert>
#include <vector>
#include <wmmintrin.h>
#include <bitset>

#include <divsufsort.h> // external library

#include "cryptography.h"
#include "misc/bitbuffer.h"
#include "misc/model.h"
#include "misc/dc3.h"

Compression::Compression( bool& aborting_variable ) :
        aborting_var(&aborting_variable)
        , text(new uint8_t[0])
        , size(0) {}


Compression::~Compression() {
    delete[] text;
}


void Compression::load_text( std::fstream &input, uint64_t text_size )
{
    if (!*aborting_var) {
        delete[] this->text;
        this->size = text_size;
        this->text = new uint8_t [this->size];
        input.read( (char*)this->text, this->size );
    }
}


void Compression::load_part(std::fstream &input, uint64_t text_size, uint32_t part_num, uint32_t block_size) {
    if (!*aborting_var) {
        delete[] this->text;
        assert( block_size * part_num <= text_size ); // part_size * part_num == starting position

        if (block_size * (part_num+1) < text_size ) this->size = block_size;
        else this->size = text_size - (block_size * part_num);

        assert( this->size <= block_size );

        this->text = new uint8_t [this->size];

        assert( input.is_open() );
        input.seekg(block_size * part_num);
        input.read( (char*)this->text, this->size );
    }
}


void Compression::save_text( std::fstream &output ) {
    if (!*aborting_var) output.write((char*)(this->text), this->size);
}


void Compression::BWT_make()
{
    if (!*aborting_var) {
        uint64_t n = this->size;
        assert(this->size > 0);

        // Generating suffix array (SA)
        uint32_t* SA = nullptr;
        dc3::BWT_DC3(text, SA, n, 0xFF);

        if (*aborting_var) {
            delete[] SA;
            return;
        }

        auto encoded = new uint8_t[n+1+4](); // +1 byte due to appending EOF during DC3
        // +4 bytes for adding uint32 EOF position during decoding at the end of encoded text

        uint32_t original_message_index = 0;
        for (uint32_t i=0; i < n+1 and !*aborting_var; ++i)
        {
            int32_t ending = SA[i]-1;
            if (ending == -1) {
                original_message_index = i; // this is where EOF would go, but we're using it only in logic
                ending = 0;        // so we change it to something that's within the text, for future compression steps
            }
            encoded[i] = this->text[ending];
        }

        delete[] SA;

        if (*aborting_var) {
            delete[] encoded;
            return;
        }

        // appending encoded text with EOF position
        for (uint8_t index=0; index < 4 and !*aborting_var; ++index)
            encoded[n+1+index] = ( original_message_index >> (index*8u)) & 0xFFu;

        // replacing this->text with encoded text
        std::swap(this->text, encoded);
        delete[] encoded;
        this->size = n+5;
    }
}


void Compression::BWT_reverse()
{   // Using L-PBKDF2_HMAC_SHA256_get_block mapping
    if (!*aborting_var) {
        uint32_t encoded_length = this->size-4; // subtracting 4 bits due to 4 last bits being starting position

        // read EOF position from last 4 bits
        uint32_t eof_position = ((uint32_t)this->text[encoded_length]) | ((uint32_t)this->text[encoded_length+1]<<8u) | ((uint32_t)this->text[encoded_length+2]<<16u) | ((uint32_t)this->text[encoded_length+3]<<24u);

        // constructing counter of sign occurrences
        uint32_t SC[256];
        for (auto & i : SC) i = 0;


        // tells us how many letters letters 'x' occurred before text[x]
        auto enumeration = new uint32_t [encoded_length]();

        for (uint32_t i=0; i < eof_position and !*aborting_var; ++i) {
            enumeration[i] = SC[this->text[i]];
            SC[this->text[i]]++;
        }
        // skipping EOF
        for (uint32_t i=eof_position+1; i < encoded_length and !*aborting_var; ++i) {
            enumeration[i] = SC[this->text[i]];
            SC[this->text[i]]++;
        }


        if (*aborting_var) {
            delete[] enumeration;
            return;
        }


        uint64_t sumSC[256];   // tells you where first occurence of given char would be in sorted order
        sumSC[0] = 1;          // before char(0), there was EOF (in logic, not in memory, but we need to skip it anyway)
        for (uint16_t i=1; i < 256; ++i) sumSC[i] = sumSC[i-1] + SC[i-1];

        uint32_t decoded_length = encoded_length-1;
        auto decoded = new uint8_t [decoded_length]();


        if (*aborting_var) {
            delete[] enumeration;
            delete[] decoded;
            return;
        }

        decoded[decoded_length-1] = this->text[0];
        uint32_t next_sign_index = 0;

        for (uint32_t i=1; i < decoded_length and !*aborting_var; ++i ) { // starts from 1, because we already decoded first sign above

            uint8_t previous_sign = this->text[next_sign_index];
            next_sign_index = sumSC[previous_sign] + enumeration[next_sign_index];

            decoded[decoded_length-i-1] = this->text[next_sign_index];
        }

        delete[] enumeration;

        if (*aborting_var) {
            delete[] decoded;
            return;
        }

        // replacing encoded text with decoded
        std::swap(this->text, decoded);
        delete[] decoded;
        this->size = decoded_length;
    }
}


void Compression::BWT_make2()
{
    if (!*aborting_var) {
        uint64_t n = this->size;

        // Appending the text to the text, to get correct suffix array
        // Algorithm doesn't always work without this step

        auto doubletext = new uint8_t[n*2];
        for (uint32_t i=0; i < n and !*aborting_var; ++i) {
            doubletext[i] = this->text[i];
            doubletext[n+i] = this->text[i];
        }

        if (*aborting_var) {
            delete[] doubletext;
            return;
        }


        // Generating suffix array (SA)
        auto* SA = new int32_t[n*2];    // double-length suffix array for double-length text
        divsufsort(doubletext, SA, n*2);
        delete[] doubletext;

        if (*aborting_var) {
            delete[] SA;
            return;
        }


        auto* SAfiltered = new int32_t[n];  // correct text-length suffix array
        uint32_t indicesFiltered = 0;
        for (uint32_t i=0; i < n*2 and !*aborting_var; ++i) {  // loop that filters off elements of SA pointing to parts of the text after n-th character
            if (SA[i] < n) {
                SAfiltered[indicesFiltered] = SA[i];
                indicesFiltered++;
            }
        }
        delete[] SA;

        if (*aborting_var) {
            delete[] SAfiltered;
            return;
        }


        uint32_t original_message_index = 0;

        auto encoded = new uint8_t[n+4]; // +4 bytes for adding uint32 starting position during decoding at the end of encoded text
        for (uint32_t i=0; i < n and !*aborting_var; ++i) {
            int32_t ending = SAfiltered[i]-1;
            if (ending == -1) {
                ending = n-1;
                original_message_index = i; // finding row without any shift for the purpose of decoding BWT without using EOF sign
            }
            encoded[i] = this->text[ending];
        }
        delete[] SAfiltered;

        if (*aborting_var) {
            delete[] encoded;
            return;
        }

        // appending encoded text with starting position
        for (uint8_t index=0; index < 4 and !*aborting_var; index++)
            encoded[n+index] = ( original_message_index >> (index*8u)) & 0xFFu;

        // replacing this->text with encoded text
        delete[] this->text;
        this->text = new uint8_t [n+4];

        for (uint32_t i=0; i < n+4; ++i) {
            this->text[i] = encoded[i];
        }

        this->size = n+4;
    }
}


void Compression::BWT_reverse2()
{   // Using L-PBKDF2_HMAC_SHA256_get_block mapping

    if (!*aborting_var) {
        uint32_t encoded_length = this->size-4; // subtracting 4 bits due to 4 last bits being starting position

        // constructing counter of sign occurrences
        uint32_t SC[256];
        for (auto & i : SC) i = 0;
        auto enumeration = new uint32_t [encoded_length];

        for (uint32_t i=0; i < encoded_length and !*aborting_var; ++i) {
            enumeration[i] = SC[this->text[i]];
            SC[this->text[i]]++;
        }

        if (*aborting_var) {
            delete[] enumeration;
            return;
        }

        auto decoded = new uint8_t [encoded_length];

        uint64_t sumSC[256];    // sums of SC from 0 to n-1
        sumSC[0] = 0;
        for (uint16_t i=1; i < 256; ++i) sumSC[i] = sumSC[i-1] + SC[i-1];

        // read starting position from last 4 bits
        uint32_t next_sign_index = ((uint32_t)this->text[encoded_length]) | ((uint32_t)this->text[encoded_length+1]<<8u) | ((uint32_t)this->text[encoded_length+2]<<16u) | ((uint32_t)this->text[encoded_length+3]<<24u);

        if (*aborting_var) {
            delete[] enumeration;
            delete[] decoded;
            return;
        }

        decoded[encoded_length-1] = this->text[next_sign_index];

        for (uint32_t decoded_length=1; decoded_length < encoded_length and !*aborting_var; ++decoded_length ) { // starts from 1, because we already decoded first sign above

            uint8_t previous_sign = this->text[next_sign_index];
            next_sign_index = sumSC[previous_sign] + enumeration[next_sign_index];

            decoded[encoded_length-decoded_length-1] = this->text[next_sign_index];
        }

        delete[] enumeration;

        if (*aborting_var) {
            delete[] decoded;
            return;
        }

        delete[] this->text;
        this->text = new uint8_t [encoded_length];

        for (uint32_t i=0; i<encoded_length; i++ ) {
            this->text[i] = decoded[i];
        }
        delete[] decoded;
        this->size = encoded_length;
    }
}


void Compression::MTF_make()
{
    if (!*aborting_var) {
        uint32_t textlength = this->size;

        bool letter_found[256];
        for (auto &i: letter_found) i = false;


        // scanning text to get what letters are in it
        uint16_t letters_found_counter = 0;
        for (uint32_t i=0; i < textlength; ++i) {
            if ( !letter_found[this->text[i]] ) {
                letter_found[this->text[i]] = true;
                letters_found_counter++;

                if (*aborting_var) break;

                if (letters_found_counter == 256) break; // no need to check any further if all possible chars have been found
            }
        }

        if (*aborting_var) return;

        // making alphabet list
        std::list<uint8_t> alphabet;
        for (uint16_t i=0; i < 256; i++) {
            if (letter_found[i]) {
                alphabet.emplace_back(i);
            }
        }

        auto output = new uint8_t [textlength+32];  //+256 bits appended to include alphabet after encoded data



        for (uint32_t i=0; i < textlength and !*aborting_var; i++) {
            // finding current letter's place in alphabet
            uint16_t letter_position = 0;
            std::_List_iterator<uint8_t> iter = alphabet.begin();
            while( *iter != this->text[i] ) {
                letter_position++;
                iter++;
            }
            // moving said letter to front

            if ( iter != alphabet.begin() ) {
                alphabet.erase(iter);
                alphabet.push_front(this->text[i]);
            }

            // writing letter to output
            output[i] = letter_position;
        }

        if (*aborting_var) {
            delete[] output;
            return;
        }



        for ( uint32_t i=0; i < 32; ++i ) {
            uint16_t alphabet_data=0;
            for ( uint16_t k=0; k < 8; ++k ) {
                alphabet_data <<=1u;
                if (letter_found[i*8+k]) alphabet_data++;
                else if ( (alphabet_data & 0x01u) == 1u ) alphabet_data--;
            }
            output[textlength+i] = (uint8_t)(alphabet_data & 0xFFu);
        }

        if (*aborting_var) {
            delete[] output;
            return;
        }

        delete[] this->text;
        this->text = new uint8_t [textlength+32];

        for (uint32_t i=0; i < textlength+32 and !*aborting_var; ++i) this->text[i] = output[i];
        delete[] output;
        this->size = textlength+32;
    }
}


void Compression::MTF_reverse()
{
    if (!*aborting_var) {
        uint32_t textlength = this->size-32;

        // interpreting alphabet information from last 256 bits of encoded data
        std::list<uint8_t> alphabet;
        for ( uint32_t i=0; i < 32; ++i ) {
            uint8_t alphabet_data=this->text[textlength+i];
            for ( uint16_t k=8; k >= 1; --k ) {
                if (( ( alphabet_data >> (k-1u) ) & 0x01u) == 1 )  alphabet.emplace_back( i*8 + 8-k);   // may or may not be broken in several ways
            }
        }

        if (*aborting_var) return;

        auto output = new uint8_t [textlength];

        for (uint32_t i=0; i < textlength and !*aborting_var; i++) {
            // finding current letter's place in alphabet
            std::_List_iterator<uint8_t> iter = alphabet.begin();
            for (uint16_t j=0; j < this->text[i]; ++j) {
                iter++;

            }
            // moving said letter to front

            if ( iter != alphabet.begin() ) {
                alphabet.push_front(*iter);
                alphabet.erase(iter);
            }

            //writing letter to output

            output[i] = *alphabet.begin();
        }

        if (*aborting_var) {
            delete[] output;
            return;
        }

        delete[] this->text;
        this->text = new uint8_t [textlength];

        for (uint32_t i=0; i < textlength; ++i) this->text[i] = output[i];
        delete[] output;

        this->size = textlength;
    }
}


void Compression::RLE_make()
{
    if (!*aborting_var) {
        uint32_t textlength = 1+this->size; // first bit encodes whether or not RLE was used

        std::string output;
        output += (char)0xFF;   //indication that RLE was indeed used
        bool RLE_used = true;

        uint8_t counter = 0;
        for (uint32_t i=1; i<textlength and !*aborting_var; ++i) {
            counter++;
            if (this->text[i-1] != this->text[i] or counter == 255) {
                output += this->text[i-1];
                output += counter;
                counter = 0;
            }
        }

        if (*aborting_var) return;

        if ( output.length()*3 > this->size*2 ) {    // if RLE improves compression by less than 1/3 this-size bytes, then:
            RLE_used = false;
        }

        if( RLE_used ) {
            if (counter != 0) {
                output += this->text[textlength-1];
                output += counter;
            }

            delete[] this->text;
            this->text = new uint8_t [output.length()];
            for (uint32_t i=0; i<output.length() and !*aborting_var; ++i) this->text[i] = output[i];

            this->size = output.length();
        }
        else {
            auto temp = new uint8_t [1+this->size];
            temp[0] = 0x00;
            for (uint32_t i=0; i < this->size and !*aborting_var; ++i) temp[i+1] = this->text[i];
            delete[] this->text;
            this->text = new uint8_t [1+this->size];
            for (uint32_t i=0; i < 1+this->size and !*aborting_var; ++i) this->text[i] = temp[i];
            delete[] temp;
            this->size++;
        }
    }
}


void Compression::RLE_reverse()
{
    if (!*aborting_var) {
        if (this->text[0] == 0xFF) {

            std::string output;
            for (uint32_t i=1; i < this->size and !*aborting_var; i+=2 ) {
                for (uint32_t j=0; j < this->text[i+1]; j++ ) output += this->text[i];
            }

            if (*aborting_var) return;

            delete[] this->text;
            this->text = new uint8_t [output.length()];
            for (uint32_t i=0; i<output.length() and !*aborting_var; ++i) this->text[i] = output[i];

            this->size = output.length();
        }
        else if (this->text[0] == 0x00) {
            auto temp = new uint8_t [this->size-1];
            for (uint32_t i=0; i<this->size-1 and !*aborting_var; ++i) temp[i] = this->text[i+1];

            if (*aborting_var) {
                delete[] temp;
                return;
            }

            delete[] this->text;
            this->text = new uint8_t [this->size-1];
            for (uint32_t i=0; i<this->size-1 and !*aborting_var; ++i) this->text[i] = temp[i];
            delete[] temp;
            this->size--;

        }
        else throw std::invalid_argument("RLE was neither used nor not used, apparently");
    }
}


void Compression::RLE_makeV2()
{
    if (!*aborting_var) {
        uint32_t textlength = 1+this->size; // first bit encodes whether or not RLE was used

        std::string output_run_length;
        std::string output_chars;
        output_run_length += (char)0xFF;   //indication that RLE was indeed used
        bool RLE_used = true;

        uint8_t counter = 0;
        for (uint32_t i=1; i<textlength; ++i) {
            counter++;
            if (this->text[i-1] != this->text[i] or counter == 255) {
                output_chars += this->text[i-1];
                output_run_length += counter;
                counter = 0;
                if (*aborting_var) return;
            }
        }

        if ( (output_chars.length() + output_run_length.length())*3 > this->size*2 ) {    // if RLE improves compression by less than 1/3 this-size bytes, then:
            RLE_used = false;
        }

        if( RLE_used ) {
            if (counter != 0) {
                output_chars += this->text[textlength-1];
                output_run_length += counter;
            }

            delete[] this->text;
            this->text = new uint8_t [output_run_length.length() + output_chars.length()];
            for (uint32_t i=0; i<output_run_length.length(); ++i) this->text[i] = output_run_length[i];

            if (*aborting_var) return;

            for (uint32_t i=0; i<output_chars.length(); ++i) this->text[output_run_length.length()+i] = output_chars[i];

            this->size = output_run_length.length() + output_chars.length();
        }
        else {
            auto temp = new uint8_t [1+this->size];
            temp[0] = 0x00;
            for (uint32_t i=0; i < this->size; ++i) temp[i+1] = this->text[i];

            if (*aborting_var) return;

            delete[] this->text;
            this->text = new uint8_t [1+this->size];
            for (uint32_t i=0; i < 1+this->size; ++i) this->text[i] = temp[i];
            delete[] temp;
            this->size++;
        }
    }
}


void Compression::RLE_reverseV2()
{
    if (!*aborting_var) {
        if (this->text[0] == 0xFF) {

            std::string output;
            uint32_t RLE_size = (this->size-1)/2;
            for (uint32_t i=0; i < RLE_size; ++i ) {
                for (uint32_t j=0; j < this->text[1+i]; j++ ) output += this->text[1+RLE_size+i];
            }

            if (*aborting_var) return;

            delete[] this->text;
            this->text = new uint8_t [output.length()];
            for (uint32_t i=0; i<output.length(); ++i) this->text[i] = output[i];

            this->size = output.length();
        }
        else if (this->text[0] == 0x00) {
            auto temp = new uint8_t [this->size-1];
            for (uint32_t i=0; i<this->size-1; ++i) temp[i] = this->text[i+1];

            if (*aborting_var) return;

            delete[] this->text;
            this->text = new uint8_t [this->size-1];
            for (uint32_t i=0; i<this->size-1; ++i) this->text[i] = temp[i];
            delete[] temp;
            this->size--;

        }
        else throw std::invalid_argument("RLE was neither used nor not used, apparently");

    }
}


void Compression::AC_make()
{

    if (!*aborting_var) {
        //  arithmetic coding - file size version
        //  based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)

        uint64_t whole = UINT_MAX;
        long double wholed = UINT_MAX;
        uint64_t half = roundl(wholed / 2.0);
        uint64_t quarter = roundl(wholed / 4.0);


        std::vector<uint64_t> r = model::AC::memoryless(this->text, this->size);

        if (*aborting_var) return;

        uint16_t r_size = 256;
        std::string output(4+4+r_size*4, 0);    // leaving 4 bits for compressed data size


        for (uint8_t i=0; i < 4; i++) {
            output[4+i] = (uint8_t)( this->size >> (i*8u)) & 0xFFu;
        }

        std::vector<uint64_t> c;
        std::vector<uint64_t> d;
        for (uint16_t i=0; i < r_size; i++) {
            output[8 + 4 * i] = r[i] & 0xFFu;
            output[8 + 4 * i + 1] = (r[i] >> 8u) & 0xFFu;
            output[8 + 4 * i + 2] = (r[i] >> 16u) & 0xFFu;
            output[8 + 4 * i + 3] = (r[i] >> 24u) & 0xFFu;

            // generate c and d, where c[i] is lower bound to get i-th letter of our alphabet, and d[i] is the upper bound
            uint64_t sum = 0;
            std::for_each(std::begin(r), std::begin(r)+i, [&] (uint64_t j) {sum += j;} );
            c.push_back(sum);
            if (i != r_size-1)
                d.push_back(sum+r[i]);
            else
                d.push_back(whole);
        }

        //check if sum of probabilities represented as UINT_MAX is equal to whole
        if (true)
        {
            uint64_t sum = 0;
            for (auto& n : r)
                sum += n;
            assert(sum == whole);
        }

        //Actual encoding
        uint64_t a = 0;
        uint64_t b = whole;
        uint64_t w = 0;
        uint32_t s = 0;

        TextWriteBitbuffer bitout(output );



        for (uint32_t i = 0; i <this->size and !*aborting_var; ++i)
        {
            w = b - a;
            b = a + roundl(((uint64_t)(d[this->text[i]]*w))/wholed); //potential rounding errors
            a = a + roundl(((uint64_t)(c[this->text[i]]*w))/wholed);

            assert( a<whole and b<=whole );
            assert( a<b );

            while (b < half or a >= half)
            {
                if (b < half)
                {
                    bitout.add_bit_0();
                    for (uint32_t j = 0; j < s; j++) bitout.add_bit_1();

                    s = 0;
                    a *= 2;
                    b *= 2;
                }
                else if (a >= half)
                {

                    bitout.add_bit_1();
                    for (uint32_t j = 0; j < s; j++) bitout.add_bit_0();

                    s = 0;
                    a = 2 * (a-half);
                    b = 2 * (b-half);
                }
            }
            while (a >= quarter and b < 3*quarter)
            {
                s++;
                a = 2 * (a-quarter);
                b = 2 * (b-quarter);
            }
        }

        s++;
        if (a <= quarter)
        {
            bitout.add_bit_0();
            for (uint32_t j = 0; j < s; j++) bitout.add_bit_1();
        }
        else
        {
            bitout.add_bit_1();
            for (uint32_t j = 0; j < s; j++) bitout.add_bit_0();
        }

        bitout.flush();

        if (*aborting_var) return;

        for (uint8_t i=0; i < 4; i++) { // filling first 4 bits of output with compressed data size in   B I T S
            output[i] = (uint8_t)( (uint32_t)bitout.get_output_size() >> (i*8u)) & 0xFFu;
        }

        this->size = output.length();

        delete[] this->text;
        this->text = new uint8_t [this->size];

        for (uint32_t i=0; i < this->size; ++i) this->text[i] = output[i];
    }
}


void Compression::AC_reverse()
{

    if (!*aborting_var) {
        uint16_t precision = 31;
        uint64_t whole = UINT_MAX;
        long double wholed = UINT_MAX;
        uint64_t half = roundl(wholed/2.0);
        uint64_t quarter = roundl(wholed/4.0);

        uint16_t r_size = 256;


        uint64_t sum = 0;
        std::vector<uint64_t> c;
        std::vector<uint64_t> d;
        c.push_back(0);
        std::string alphabet;

        uint32_t compressed_file_size = ((uint32_t)this->text[0]) | ((uint32_t)this->text[1]<<8u) | ((uint32_t)this->text[2]<<16u) | ((uint32_t)this->text[3]<<24u);
        uint32_t uncompressed_file_size = ((uint32_t)this->text[4]) | ((uint32_t)this->text[5]<<8u) | ((uint32_t)this->text[6]<<16u) | ((uint32_t)this->text[7]<<24u);

        uint8_t r_buffer[256*4];
        for (uint16_t i=0; i<r_size*4; ++i) r_buffer[i] = this->text[i+8];  // r array starts after 8 bytes
        for ( uint16_t i=0; i < r_size; i++ )
        {
            uint64_t p1 = r_buffer[i * 4];
            uint64_t p2 = (r_buffer[i * 4 + 1] << 8u);
            uint64_t p3 = (r_buffer[i * 4 + 2] << 16u);
            uint64_t p4 = ((uint64_t)r_buffer[i * 4 + 3] << 24u);
            assert(p1 < whole and p2 < whole and p3 < whole and p4 < whole);
            uint64_t rn = p1 | p2 | p3 | p4;

            if (rn != 0) {
                alphabet += char(i);
                sum += rn;
                if (i != 255) c.push_back(sum);
                d.push_back(sum);
            }
        }

        TextReadBitbuffer in_bit(this->text, compressed_file_size, 4+4+256*4);

        uint64_t a = 0;
        uint64_t b = whole;
        uint64_t z = 0;
        uint64_t w = 0;


        // actual decoding

        uint64_t i = 0;
        while ( i<=precision and i < compressed_file_size )
        {
            if (in_bit.getbit()) z += (1ull<<(precision-i)); //if bit's value == 1
            i++;
        }


        std::string output;


        uint64_t a0 = 0;
        uint64_t b0 = 0;

        uint64_t output_size_counter = 0;

        if (*aborting_var) return;

        while (!*aborting_var)
        {
            w = b - a;
            assert( z>=a and w<=whole );

            uint64_t searched = floorl((long double)(z - a) * wholed / (long double)w);
            int16_t j = std::upper_bound(d.begin(), d.end(), searched ) - d.begin();

            b0 = a + roundl(d[j] * w / wholed); //potential rounding errors
            if (b0 <= z) {
                j++;
                b0 = a + roundl(d[j] * w / wholed); //potential rounding errors
            }
            a0 = a + roundl(c[j] * w / wholed);
            assert( j>=0 and j < alphabet.length() );
            assert(((a0 <= z) and (z < b0)));
            assert(a0 < whole and b0 <= whole);
            assert(a <= whole);
            assert(b <= whole);
            assert(a < b);
            assert(a0 < b0);

            output += alphabet[j];

            output_size_counter++;

            a = a0;
            b = b0;
            if (output_size_counter == uncompressed_file_size) {
                break;
            }

            while (b < half or a >= half)
            {
                if (b < half)
                {
                    a <<= 1u;
                    b <<= 1u;
                    z <<= 1u;
                }
                else if (a >= half)
                {
                    a = (a-half)<<1u;
                    b = (b-half)<<1u;
                    z = (z-half)<<1u;
                }

                if (i < compressed_file_size ) {
                    if (in_bit.getbit()) z++;
                    i++;
                }
            }
            while (a >= quarter and b < 3*quarter)
            {
                a = (a-quarter)<<1u;
                b = (b-quarter)<<1u;
                z = (z-quarter)<<1u;
                if (i < compressed_file_size) {
                    if (in_bit.getbit()) z++;
                    i++;
                }
            }
        }

        if (*aborting_var) return;
        this->size = output.length();
        delete[] this->text;
        this->text = new uint8_t [this->size];

        for (uint32_t j=0; j < this->size; ++j) this->text[j] = output[j];

    }
}


void Compression::AC2_make() {

    //  arithmetic coding - file size version
    //  based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)

    if (!*aborting_var) {
        uint64_t whole = UINT_MAX;
        long double wholed = UINT_MAX;
        uint64_t half = roundl(wholed / 2.0);
        uint64_t quarter = roundl(wholed / 4.0);


        std::vector<std::vector<uint32_t>> rr = model::AC::order_1(this->text, this->size);

        if (*aborting_var) return;

        uint16_t r_size = 256;
        std::string output(4 + 4 + r_size * r_size * 4, 0);    // leaving 4 bits for compressed data size


        for (uint8_t i = 0; i < 4; i++) {
            output[4 + i] = (uint8_t) (this->size >> (i * 8u)) & 0xFFu;
        }

        std::vector<std::vector<uint64_t>> c;
        std::vector<std::vector<uint64_t>> d;

        for (uint16_t r = 0; r < r_size; r++) {
            c.emplace_back(257);
            d.emplace_back(257);
            c[r][0] = 0;

            for (uint16_t i = 0; i < r_size; i++) { // saving probabilites to file, 256*256*4 bytes, unfortunately
                if (rr[r][i] != 0) {
                    output[8 + r * 256 * 4 + 4 * i] = rr[r][i] & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 1] = (rr[r][i] >> 8u) & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 2] = (rr[r][i] >> 16u) & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 3] = (rr[r][i] >> 24u) & 0xFFu;
                }
                // generating partial sums c and d
                c[r][i + 1] = (c[r][i] + rr[r][i]);
                d[r][i] = c[r][i + 1];
            }
            d[r][255] = whole;
        }


        //check if sum of probabilities represented as UINT_MAX is equal to whole
        if (true) {
            for (auto &r : rr) {
                uint64_t sum = 0;
                for (auto &n : r) sum += n;
                assert(sum == whole or sum == 0);
            }
        }

        //Actual encoding
        uint64_t a = 0;
        uint64_t b = whole;
        uint64_t w;
        uint32_t s = 0;

        if (*aborting_var) return;

        TextWriteBitbuffer bitout(output);

        output += text[0];  //  saving first char, for decoding purposes

        for (uint32_t i = 1; i < this->size and !*aborting_var; ++i) {

            w = b - a;
            b = a + roundl(((uint64_t)(d[this->text[i-1]][this->text[i]]*w))/wholed);
            a = a + roundl(((uint64_t)(c[this->text[i-1]][this->text[i]]*w))/wholed);

            assert(a < whole and b <= whole);
            assert(a < b);
            while (b < half or a >= half) {
                if (b < half) {
                    bitout.add_bit_0();
                    for (uint32_t j = 0; j < s; j++) {
                        bitout.add_bit_1();
                    }
                    s = 0;
                    a *= 2;
                    b *= 2;
                } else if (a >= half) {
                    bitout.add_bit_1();
                    for (uint32_t j = 0; j < s; j++) {
                        bitout.add_bit_0();
                    }
                    s = 0;
                    a = 2 * (a - half);
                    b = 2 * (b - half);
                }
            }
            while (a >= quarter and b < 3 * quarter) {
                s++;
                a = 2 * (a - quarter);
                b = 2 * (b - quarter);
            }
        }

        s++;
        if (a <= quarter) {
            bitout.add_bit_0();
            for (uint32_t j = 0; j < s; j++) {
                bitout.add_bit_1();
            }
        } else {
            bitout.add_bit_1();
            for (uint32_t j = 0; j < s; j++) {
                bitout.add_bit_0();
            }
        }

        bitout.flush();



        if (*aborting_var) return;

        for (uint8_t i = 0; i < 4; i++) { // filling first 4 bits of output with compressed data size in   B I T S
            output[i] = (uint8_t) ((uint32_t) bitout.get_output_size() >> (i * 8u)) & 0xFFu;
        }

        this->size = output.length();

        delete[] this->text;
        this->text = new uint8_t[this->size];

        for (uint32_t i = 0; i < this->size; ++i) this->text[i] = output[i];
    }
}


void Compression::AC2_reverse()
{
    if (!*aborting_var) {
        uint16_t precision = 31;
        uint64_t whole = UINT_MAX;
        long double wholed = UINT_MAX;
        uint64_t half = roundl(wholed/2.0);
        uint64_t quarter = roundl(wholed/4.0);

        uint16_t r_size = 256;

        std::vector<std::vector<uint64_t>> c(256);
        std::vector<std::vector<uint64_t>> d(256);
        std::vector<std::string> alphabet(256);

        uint32_t compressed_file_size = ((uint32_t)this->text[0]) | ((uint32_t)this->text[1]<<8u) | ((uint32_t)this->text[2]<<16u) | ((uint32_t)this->text[3]<<24u);
        uint32_t uncompressed_file_size = ((uint32_t)this->text[4]) | ((uint32_t)this->text[5]<<8u) | ((uint32_t)this->text[6]<<16u) | ((uint32_t)this->text[7]<<24u);

        for (uint16_t r = 0; r < r_size; ++r) {
            std::vector<uint64_t> temp_c(1,0);
            std::vector<uint64_t> temp_d;

            uint64_t sum = 0;
            uint8_t r_buffer[256 * 4];
            for (uint16_t i = 0; i < r_size * 4; ++i) r_buffer[i] = this->text[8 + r*r_size*4 + i];  // r array starts after 8 bytes
            for (uint16_t i = 0; i < r_size; i++) {
                uint64_t p1 = r_buffer[i * 4];
                uint64_t p2 = (r_buffer[i * 4 + 1] << 8u);
                uint64_t p3 = (r_buffer[i * 4 + 2] << 16u);
                uint64_t p4 = ((uint64_t) r_buffer[i * 4 + 3] << 24u);
                assert(p1 < whole and p2 < whole and p3 < whole and p4 < whole);
                uint64_t rn = p1 | p2 | p3 | p4;

                if (rn != 0) {
                    alphabet[r] += char(i);
                    sum += rn;
                    temp_d.push_back(sum);
                    if ( i!=255 ) {
                        temp_c.push_back(sum);
                    }
                }
            }
            if (!temp_d.empty()) {
                c[r] = temp_c;
                d[r] = temp_d;
            }
        }

        if (*aborting_var) return;

        TextReadBitbuffer in_bit(this->text, compressed_file_size, 8+256*256*4+1);

        uint64_t a = 0;
        uint64_t b = whole;
        uint64_t z = 0;
        long double w = 0;

        // actual decoding

        uint64_t i = 0;
        while ( i<=precision and i < compressed_file_size )
        {
            if (in_bit.getbit()) //if bit's value == 1
            {
                z += (1ull<<(precision-i));
            }
            i++;
        }

        std::string output;


        uint64_t a0 = 0;
        uint64_t b0 = 0;


        uint8_t previous_char = this->text[8+r_size*r_size*4];    // first char of decoded text is normal ascii char
        output += previous_char;
        uint64_t output_size_counter = 1;


        while (output_size_counter != uncompressed_file_size and !*aborting_var)
        {
            assert( z <= b );

            w = b - a;
            assert( z>=a and w<=whole );
            uint64_t searched = floorl((long double)(z - a) * wholed / (long double)w);

            int16_t j = std::upper_bound(d[previous_char].begin(), d[previous_char].end(), searched ) - d[previous_char].begin();

            b0 = a + roundl(d[previous_char][j] * w / wholed); //potential rounding errors
            if (b0 <= z) {
                j++;
                b0 = a + roundl(d[previous_char][j] * w / wholed); //potential rounding errors
            }
            a0 = a + roundl(c[previous_char][j] * w / wholed);

            assert( j>=0 and j < alphabet[previous_char].length() );
            assert( c[previous_char][j] < d[previous_char][j] );
            assert(((a0 <= z) and (z < b0)));
            assert(a0 <= whole and b0 <= whole);
            assert(a <= whole);
            assert(b <= whole);
            assert(a < b);
            assert(a0 < b0);

            output += alphabet[previous_char][j];
            previous_char = alphabet[previous_char][j];

            output_size_counter++;

            a = a0;
            b = b0;

            while (b < half or a >= half)
            {
                if (b < half)
                {
                    a *= 2;
                    b *= 2;
                    z *= 2;

                }
                else if (a >= half)
                {
                    a = (a-half)*2;
                    b = (b-half)*2;
                    z = (z-half)*2;
                }

                if (i < compressed_file_size ) {
                    if (in_bit.getbit()) z++;
                    i++;
                }
                assert( z <= b );
            }
            while (a >= quarter and b < 3*quarter)
            {
                a = (a-quarter)*2;
                b = (b-quarter)*2;
                z = (z-quarter)*2;
                if (i < compressed_file_size) {
                    if ( in_bit.getbit() ) z++;
                    i++;
                }
                assert( z <= b );
            }
        }

        if (*aborting_var) return;

        this->size = output.length();
        delete[] this->text;
        this->text = new uint8_t [this->size];

        for (uint32_t j=0; j < this->size; ++j) this->text[j] = output[j];
    }
}

void Compression::rANS_make()
{
    // Loosely based on a paper by James Townsend at https://arxiv.org/pdf/2001.09186.pdf

    std::vector<uint64_t> PMF;                        // PMF - Probability Mass Function
    uint8_t index_of_chars[256] = {0x00};             // given real char, gives us its index in PMF and CMF

    {   // brackets here are purely to avoid pollution
        PMF = model::ANS::memoryless(text, size);   // Calculate the model for PMF
        std::vector<uint64_t> trimmed_PMF;  // We'll kick out all the probabilities = 0 for
        // faster binary search during decoding

        uint8_t current_identifier = 0;
        for (uint16_t i = 0; i < 256; ++i) {
            if (PMF[i] != 0) {
                trimmed_PMF.push_back(PMF[i]);
                index_of_chars[i] = current_identifier;
                current_identifier++;
            }
        }

        std::swap(trimmed_PMF, PMF);
    }

    std::vector<uint64_t> CMF(PMF.size() + 1, 0);   // CMF - Cumulative Mass Function
    for (uint32_t i=0; i < PMF.size(); ++i) {
        CMF[i + 1] = CMF[i] + PMF[i];                   // calculating cumulative probabilities
    }

    uint64_t state = 1l << 32;                          // state

    auto* stack = new uint32_t [size + 10]();           // stack of encoded states
    uint32_t stack_i = 0;                               // stack size / iterator

    uint32_t counter[256] = {};
    for (int64_t i=size-1; i >=0; --i) {
        ++counter[text[i]];                             // counting chars for the decoder

        uint8_t x = index_of_chars[text[i]];            // index of char in our trimmed probability vectors
        uint64_t p_i = PMF[x];
        uint64_t prob = (p_i << 32) - 1;                // -1 in case p_i == 1<<32
                                                        // (happens for all sources using only 1 symbol)

        // rescaling & saving
        while (state >= prob) {
            stack[stack_i++] = state & 0xFFFFFFFF;      // saving state
            state >>= 32;
        }

        state = ((state / p_i) << 32) + state % p_i + CMF[x];   // next state
    }
    // last state also needs to be pushed to stack
    stack[stack_i++] = state & 0xFFFFFFFF;
    stack[stack_i++] = (state >> 32) & 0xFFFFFFFF;

    // creating output from original_size + model + stack

    // model layout: [what symbols were used][how many of each was used]
    // that means it'state going to be recalculated during decoding

    uint32_t used_chars_size = 32;
    uint32_t model_size = PMF.size() * 3; // *3, because we'll be saving the model on 3 bytes per element
    // PMF.size() works, because PMF contains probabilities of only the chars, which occurred in the text

    uint32_t stack_size = stack_i * sizeof(stack[0]);   // size of stack
    uint32_t output_size = sizeof(size) + used_chars_size + model_size + stack_size;

    auto* output = new uint8_t [output_size]();
    uint32_t output_i = 0;

    // saving size (4 bytes)
    *reinterpret_cast<uint32_t*>(output) = size;
    output_i += 4;

    // preparing information about what chars were used and saving it
    std::bitset<64> used_chars(0);
    for (uint32_t i=0; i < 4; ++i) {
        for (uint32_t j=0; j < 64; ++j) {
            if (counter[i*64 + j] != 0) {
                used_chars[j] = true;
            }
        }
        *reinterpret_cast<uint64_t*>(output + output_i + i*8) = used_chars.to_ullong();
        used_chars.reset();
    }

    output_i += used_chars_size;

    // saving char counts
    uint32_t saved = 0;
    for (uint32_t i=0; i < 256; ++i) {
        if (counter[i] != 0) {
            for (int j=0; j < 3; ++j) { // 24 bits will be enough, since block size is limited to at most 16 MiB
                *(output + output_i + (saved)*3 +j) = *((uint8_t*)&counter + 4*i+j);
            }
            ++saved;
        }
    }
    output_i += saved*3;

    assert(output_i == sizeof(size) + used_chars_size + model_size);


    // saving states produced by the encoder
    for (uint32_t i=0; i < stack_i; ++i) {
        *reinterpret_cast<uint32_t*>(output + output_i + i*4) = stack[i];
    }

    std::swap(text, output);
    std::swap(size, output_size);

    delete[] output;
    delete[] stack;
}


void Compression::rANS_reverse()
{
    // Loosely based on a paper by James Townsend at https://arxiv.org/pdf/2001.09186.pdf

    // loading original size
    uint32_t original_size = *reinterpret_cast<uint32_t*>(text);

    // loading info about which chars were used
    bool char_used[256] = {};
    uint16_t used_char_count = 0;   // contains info about how many different chars were used
    for (int i=0; i < 4; ++i) {
        std::bitset<64> used(*reinterpret_cast<uint64_t*>(text+4+8*i));
        for (int j=0; j < 64; ++j) {
            char_used[i*64 + j] = used[j];
        }
        used_char_count += used.count();
    }

    // now that we know how many char counts to expect, we can load them, and prepare char2index table
    auto* index2char = new uint8_t [used_char_count]();

    uint16_t used_found = 0;
    std::vector<uint64_t> PMF;  // PMF - Probability Mass Function
    // we'll load char counts into PMF and then, normalize them to get PMF used during encoding

    for (int i=0; i < 256; ++i) {
        if (char_used[i]) {
            // character counts are 3 bytes wide, so we'll mask off the 4th byte
            PMF.emplace_back(*reinterpret_cast<uint32_t*>(text + 4 + 32 + 3 * used_found) & 0x00FFFFFF);

            index2char[used_found] = i;
            ++used_found;
        }
    }
    const uint64_t limit = 1ull << 32;

    model::ANS::normalize_frequencies(PMF, limit);

    std::vector<uint64_t> CMF(used_found + 1, 0);   // CMF - Cumulative Mass Function
    for (int i=0; i < PMF.size(); ++i) CMF[i + 1] = CMF[i] + PMF[i];

    auto* stack = reinterpret_cast<uint32_t*>(text + sizeof(size) + 32 + 3 * used_found);
    uint32_t stack_i = (size - (4 + 32 + 3 * used_found)) / 4;

    // loading the last state during encoding
    uint64_t state = stack[--stack_i];
    state <<= 32;
    state += stack[--stack_i];


    auto* decoded = new uint8_t [original_size]();

    // decoding
    for (uint32_t it=0; it < original_size; ++it) {
        uint64_t state_mod = state & 0xFFFFFFFF;    // equal to state % limit, but faster
        // Using reminder from division of state by limit, we can determine which sign was used to generate this state,
        // by looking into which cumulative probability it falls into
        uint64_t i = std::upper_bound(CMF.begin(), CMF.end(), state_mod) - CMF.begin() - 1;    // crime against performance

        uint64_t previous_state = PMF[i] * (state >> 32) + state_mod - CMF[i]; // this is somehow faster than just overwriting state
        // at least according to cachegrind

        // if state has space for another element from stack (likely)
        if (previous_state < limit) {
            previous_state = (previous_state << 32) + stack[--stack_i];
            // while state has space for another element from stack (unlikely)
            while (previous_state < limit) {
                // pop an element t_top from stack stack
                // and push t_top into the lower bits of state
                previous_state = (previous_state << 32) + stack[--stack_i];
            }
        }
        state = previous_state;

        decoded[it] = index2char[i];    // retranslating index extracted from state into the usual ASCII
    }

    std::swap(decoded, text);
    std::swap(size, original_size);

    delete[] decoded;
    delete[] index2char;
}


void Compression::AES128_make(uint8_t key[], uint32_t key_size, uint8_t iv[], uint32_t iv_size, uint8_t metadata[], uint8_t metadata_size) {

    uint64_t original_size = this->size;
    crypto::AES128::encrypt(this->text, original_size, key, key_size, iv, iv_size);
    this->size = original_size;
    if (this->part_id == 0) {   // if this is the 1st block, we'll need to add the metadata necessary for decryption
        assert(metadata != nullptr);
        assert(metadata_size != 0);

        uint32_t prepended_size = metadata_size + this->size;
        auto* prepended_with_metadata = new uint8_t [prepended_size];

        // prepending ciphertext with metadata
        for (uint32_t i=0; i < metadata_size; ++i) {
            prepended_with_metadata[i] = metadata[i];
        }

        for (uint64_t i = 0; i < this->size; ++i ) {
            prepended_with_metadata[metadata_size + i] = this->text[i];
        }

        std::swap(this->text, prepended_with_metadata);
        std::swap(this->size, prepended_size);
        delete[] prepended_with_metadata;
    }


}


void Compression::AES128_reverse(uint8_t key[], uint32_t key_size) {
    uint64_t temp_size = this->size;    // without this, casting the value to uint64_t will cause a bug
    crypto::AES128::decrypt(this->text, temp_size, key, key_size);
    this->size = temp_size;
}


void Compression::AES128_extract_metadata(uint8_t *&metadata, uint32_t &metadata_size) {
    assert(part_id == 0);

    metadata_size = proper_metadata_size;
    metadata = new uint8_t [metadata_size];

    for (uint32_t i=0; i < metadata_size; ++i) {
        metadata[i] = this->text[i];
    }

    for (uint32_t i = metadata_size; i < this->size; ++i) {
        this->text[i-metadata_size] = this->text[i];
    }
    this->size -= metadata_size;
}


bool Compression::AES128_verify_password_str(std::string& pw, uint8_t *metadata, uint32_t metadata_size) {
    assert(metadata_size == proper_metadata_size);
    const uint32_t aes128_key_size = 16;

    // we'll need to parse metadata to verify the password
    uint32_t salt_size = crypto::PBKDF2::saltSize;
    uint8_t salt[salt_size];

    uint64_t loading_offset = 0;
    for (uint64_t i=0; i < salt_size; ++i) {                            // loading salt
        salt[i] = metadata[i];
    }
    loading_offset += salt_size;

    const uint32_t iterations_size = 8;
    uint64_t iterations = 0;        // needs to be 8 bytes long!


    for (uint64_t i=0; i < iterations_size; ++i) {                      // loading the number of iterations
        iterations += ((uint64_t)metadata[loading_offset + i] << (i * 8u));
    }
    loading_offset += iterations_size;
    std::cout << "Iterations: " << iterations << std::endl;

    std::cout << "Deriving key...\t" << std::flush;
    std::string pkey = crypto::PBKDF2::HMAC_SHA256(pw, salt, salt_size, iterations, aes128_key_size, *this->aborting_var);
    std::cout << "Done" << std::endl;

    uint64_t encrypted_key_size = 16+16;    // 0-15 - IV, 16-31 - ciphertext
    auto* encrypted_key = new uint8_t [encrypted_key_size];
    for (uint32_t i=0; i < encrypted_key_size; ++i) {
        encrypted_key[i] = metadata[loading_offset++];
    }


    crypto::AES128::decrypt(encrypted_key, encrypted_key_size, (uint8_t*) pkey.c_str(), pkey.length());

    uint32_t hmac_size = 32;
    std::string generated_hmac = crypto::HMAC::SHA256(metadata + salt_size + iterations_size, hmac_size,
                                                      encrypted_key, encrypted_key_size, *this->aborting_var);

    auto* hmac = new uint8_t [hmac_size];


    for (uint64_t i=0; i < hmac_size; ++i) {                            // loading HMAC-SHA256
        hmac[i] = metadata[loading_offset++];
    }

    bool correct_password = true;

    for (uint32_t i=0; i < hmac_size; ++i){
        if (hmac[i] != (uint8_t) generated_hmac[i]) {
            correct_password = false;
            break;
        }
    }

    delete[] hmac;
    delete[] encrypted_key;
    return correct_password;
}






