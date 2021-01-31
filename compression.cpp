#include "compression.h"

#include <iostream>
#include <chrono>
#include <list>
#include <climits>
#include <cmath>
#include <cassert>
#include <vector>

#include <divsufsort.h> // external library


Compression::Compression( bool& aborting_variable ) {
    this->aborting_var = &aborting_variable;
    text = new uint8_t [0];
    size = 0;
}


Compression::~Compression() {
    delete[] text;
}


void Compression::load_text( std::fstream &input, uint64_t text_size ) {
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
        assert( block_size * part_num < text_size ); // part_size * part_num == starting position

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


void Compression::BWT_reverse()
{   // Using L-F mapping

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


        statistical_tools st;
        st.iid_model_from_text( this->text, this->size );

        if (*aborting_var) return;

        uint16_t r_size = 256;
        std::string output(4+4+r_size*4, 0);    // leaving 4 bits for compressed data size


        for (uint8_t i=0; i < 4; i++) {
            output[4+i] = (uint8_t)( this->size >> (i*8u)) & 0xFFu;
        }

        std::vector<uint64_t> c;
        std::vector<uint64_t> d;
        for (uint16_t i=0; i < r_size; i++) {
            output[8 + 4 * i] = st.r[i] & 0xFFu;
            output[8 + 4 * i + 1] = (st.r[i] >> 8u) & 0xFFu;
            output[8 + 4 * i + 2] = (st.r[i] >> 16u) & 0xFFu;
            output[8 + 4 * i + 3] = (st.r[i] >> 24u) & 0xFFu;

            // generate c and d, where c[i] is lower bound to get i-th letter of our alphabet, and d[i] is the upper bound
            uint64_t sum = 0;
            std::for_each(std::begin(st.r), std::begin(st.r)+i, [&] (uint64_t j) {sum += j;} );
            c.push_back(sum);
            if (i != r_size-1)
                d.push_back(sum+st.r[i]);
            else
                d.push_back(whole);
        }

        //check if sum of probabilities represented as UINT_MAX is equal to whole
        if (true)
        {
            uint64_t sum = 0;
            for (auto& n : st.r)
                sum += n;
            assert(sum == whole);
        }

        //Actual encoding
        uint64_t a = 0;
        uint64_t b = whole;
        uint64_t w = 0;
        uint32_t s = 0;

        Text_write_bitbuffer bitout(output );



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

        Text_read_bitbuffer in_bit(this->text, compressed_file_size, 4+4+256*4);

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


        statistical_tools st;
        st.model_from_text_1back(this->text, this->size);

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
                if (st.rr[r][i] != 0) {
                    output[8 + r * 256 * 4 + 4 * i] = st.rr[r][i] & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 1] = (st.rr[r][i] >> 8u) & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 2] = (st.rr[r][i] >> 16u) & 0xFFu;
                    output[8 + r * 256 * 4 + 4 * i + 3] = (st.rr[r][i] >> 24u) & 0xFFu;
                }
                // generating partial sums c and d
                c[r][i + 1] = (c[r][i] + st.rr[r][i]);
                d[r][i] = c[r][i + 1];
            }
            d[r][255] = whole;
        }


        //check if sum of probabilities represented as UINT_MAX is equal to whole
        if (true) {
            for (auto &r : st.rr) {
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

        Text_write_bitbuffer bitout(output);

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

        Text_read_bitbuffer in_bit(this->text, compressed_file_size, 8+256*256*4+1);

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


Text_write_bitbuffer::Text_write_bitbuffer(std::string &output_string) {
    this->text = &output_string;
    bits_written=0;
    clear();
}


Text_write_bitbuffer::~Text_write_bitbuffer() {
    if ((bi != 0) or (bitcounter != 0)) flush();
}


void Text_write_bitbuffer::clear() {
    for (auto &i : buffer) i = 0;
    bi = 0;
    bitcounter = 0;
}


void Text_write_bitbuffer::flush() {
    if (bitcounter > 0) text->append( (char*)buffer, bi+1 );
    else text->append( (char*)buffer, bi );
    clear();
}


void Text_write_bitbuffer::add_bit_1() {
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


void Text_write_bitbuffer::add_bit_0() {
    buffer[bi] <<= 1u;
    bitcounter++;
    bits_written++;

    if ( bitcounter == 8 ) {
        bi++;
        bitcounter = 0;
        if (bi >= 8*1024) flush();
    }
}


uint64_t Text_write_bitbuffer::get_output_size() const {
    return bits_written;
}


Text_read_bitbuffer::Text_read_bitbuffer(uint8_t compressed_text[], uint64_t compressed_size, uint64_t starting_position) {
    this->text = compressed_text;
    this->byte_index = starting_position;
    this->data_left_b = starting_position + ceil((double)compressed_size/8.0);

    this->output_bit = false;

    this->bits_in_last_byte = compressed_size % 8;
    if (compressed_size > 8) this->meaningful_bits = 8;
    else this->meaningful_bits = compressed_size;
    this->bitcounter = 0;

}


bool Text_read_bitbuffer::getbit() {
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
