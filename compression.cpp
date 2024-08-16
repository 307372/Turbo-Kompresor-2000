#include "compression.h"

#include <list>
#include <climits>
#include <cmath>
#include <cassert>
#include <vector>
#include <bitset>

#include <divsufsort.h> // external library

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


void Compression::load_text(std::fstream &input, uint64_t text_size)
{
    if (*aborting_var) return;

    delete[] this->text;
    this->size = text_size;
    this->text = new uint8_t [this->size];
    input.read( (char*)this->text, this->size );
}


void Compression::load_part(std::fstream &input, uint64_t text_size, uint32_t part_num, uint32_t block_size) {
    if (*aborting_var) return;

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


void Compression::save_text(std::fstream &output) {
    if (!*aborting_var) output.write((char*)(this->text), this->size);
}


void Compression::BWT_make()    // DC3
{
    if (*aborting_var) return;

    uint64_t n = this->size;
    assert(this->size > 0);

    // Generating suffix array (SA)
    uint32_t* SA = nullptr;
    dc3::BWT_DC3(text, SA, n, *aborting_var, 0xFF);

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


void Compression::BWT_reverse()
{   // Using L-F mapping
    if (*aborting_var) return;

    uint32_t encoded_length = size-4; // subtracting 4 bits due to 4 last bits being starting position

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


void Compression::BWT_make2()   // divsufsort
{
    if (*aborting_var) return;

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
    this->size = n+4;
    std::swap(text, encoded);
    delete[] encoded;
    this->size = n+4;
}


void Compression::BWT_reverse2()
{   // Using L-F mapping

    if (*aborting_var) return;

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

    std::swap(this->text, decoded);
    this->size = encoded_length;
    delete[] decoded;
}


void Compression::MTF_make()
{
    if (*aborting_var) return;

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

    auto output = new uint8_t [textlength+32];  // +256 bits appended to include alphabet after encoded data

    for (uint32_t i=0; i < textlength and !*aborting_var; i++) {
        // finding current letter's place in alphabet
        uint16_t letter_position = 0;
        auto iter = alphabet.begin();
        while( *iter != this->text[i] ) {
            ++letter_position;
            ++iter;
        }

        // moving said letter to front
        if ( iter != alphabet.begin() ) {   // unless it's already there
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

    // saving the information about which chars were found
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

    std::swap(text, output);
    this->size = textlength+32;
    delete[] output;
}


void Compression::MTF_reverse()
{
    if (*aborting_var) return;

    uint32_t textlength = this->size-32;

    // interpreting alphabet information from last 256 bits of encoded data
    std::list<uint8_t> alphabet;
    for ( uint32_t i=0; i < 32; ++i ) {
        uint8_t alphabet_data = this->text[textlength+i];
        for ( uint16_t k=8; k >= 1; --k ) {
            if (( alphabet_data >> (k-1u) ) & 0x01u)  alphabet.emplace_back( i*8 + 8-k);
        }
    }

    if (*aborting_var) return;

    auto output = new uint8_t [textlength];

    for (uint32_t i=0; i < textlength and !*aborting_var; i++) {
        // finding current letter's place in alphabet
        auto iter = alphabet.begin();
        for (uint16_t j=0; j < this->text[i]; ++j) {
            iter++;
        }
        // moving said letter to front

        if ( iter != alphabet.begin() ) {
            alphabet.push_front(*iter);
            alphabet.erase(iter);
        }

        // writing letter to output
        output[i] = *alphabet.begin();
    }

    if (*aborting_var) {
        delete[] output;
        return;
    }

    std::swap(text, output);
    this->size = textlength;

    delete[] output;
}


void Compression::RLE_make()
{
    if (*aborting_var) return;
    uint32_t textlength = 1 + size; // first byte encodes whether or not RLE was used

    std::string output;
    output.reserve(size);

    output += (char)0xFF;   //indication that RLE was indeed used
    bool RLE_used = true;

    uint8_t counter = 0;
    for (uint32_t i=1; i < textlength and !*aborting_var; ++i) {
        counter++;
        if (text[i-1] != text[i] or counter == 255) {
            output += text[i-1];
            output += counter;
            counter = 0;
        }
    }

    if (*aborting_var) return;

    if ( output.length()*3 > size*2 ) {    // if RLE improves compression by less than 1/3 this-size bytes, then:
        RLE_used = false;
    }

    if( RLE_used ) {
        if (counter != 0) {
            output += text[textlength-1];
            output += counter;
        }

        delete[] text;
        text = new uint8_t [output.length()];
        for (uint32_t i=0; i < output.length() and !*aborting_var; ++i) {
            text[i] = output[i];
        }

        size = output.length();
    }
    else {
        auto temp = new uint8_t [1+size];
        temp[0] = 0x00;
        for (uint32_t i=0; i < size and !*aborting_var; ++i) {
            temp[i+1] = text[i];
        }
        std::swap(text, temp);
        delete[] temp;
        size++;
    }
}


void Compression::RLE_reverse()
{
    if (*aborting_var) return;

    if (text[0] == 0xFF) {

        std::string output;
        output.reserve(size);

        for (uint32_t i=1; i < size and !*aborting_var; i+=2 ) {
            for (uint32_t j=0; j < text[i+1]; j++ ) output += text[i];
        }

        if (*aborting_var) return;

        delete[] text;
        text = new uint8_t [output.length()];
        for (uint32_t i=0; i<output.length() and !*aborting_var; ++i)
            text[i] = output[i];

        size = output.length();
    }
    else if (text[0] == 0x00) {
        auto temp = new uint8_t [size-1];
        for (uint32_t i=0; i < size-1; ++i) {
            temp[i] = text[i+1];
        }

        if (*aborting_var) {
            delete[] temp;
            return;
        }

        std::swap(temp, text);

        delete[] temp;
        size--;

    }
    else throw std::invalid_argument("RLE was neither used nor not used, apparently");
}


void Compression::RLE_makeV2()
{
    if (*aborting_var) return;

    uint32_t textlength = 1 + size; // first byte encodes whether or not RLE was used

    std::string output_run_length;
    output_run_length.reserve(size);

    std::string output_chars;
    output_chars.reserve(size);

    output_run_length += (char)0xFF;   // indication that RLE was indeed used
    bool RLE_used = true;

    uint8_t counter = 0;
    for (uint32_t i=1; i<textlength; ++i) {
        counter++;
        if (text[i-1] != text[i] or counter == 255) {
            output_chars += text[i-1];
            output_run_length += counter;
            counter = 0;
            if (*aborting_var) return;
        }
    }

    if ( (output_chars.length() + output_run_length.length())*3 > size*2 ) {    // if RLE improves compression by less than 1/3 this-size bytes, then:
        RLE_used = false;
    }

    if( RLE_used ) {
        if (counter != 0) {
            output_chars += text[textlength-1];
            output_run_length += counter;
        }

        delete[] text;
        text = new uint8_t [output_run_length.length() + output_chars.length()];
        for (uint32_t i=0; i<output_run_length.length(); ++i) {
            text[i] = output_run_length[i];
        }

        if (*aborting_var) return;

        for (uint32_t i=0; i<output_chars.length(); ++i) {
            text[output_run_length.length()+i] = output_chars[i];
        }

        size = output_run_length.length() + output_chars.length();
    }
    else {
        auto temp = new uint8_t [1 + size];
        temp[0] = 0x00;
        for (uint32_t i=0; i < size; ++i) {
            temp[i+1] = text[i];
        }

        if (*aborting_var) return;

        std::swap(temp, text);

        size++;
        delete[] temp;
    }
}


void Compression::RLE_reverseV2()
{
    if (*aborting_var) return;

    if (text[0] == 0xFF) {  // if RLE was used
        std::string output;
        output.reserve(size);

        uint32_t RLE_size = (size-1)/2;
        for (uint32_t i=0; i < RLE_size; ++i ) {
            for (uint32_t j=0; j < text[1+i]; j++ ) output += text[1+RLE_size+i];
        }

        if (*aborting_var) return;

        delete[] text;
        text = new uint8_t [output.length()];
        for (uint32_t i=0; i < output.length(); ++i) {
            text[i] = output[i];
        }

        size = output.length();
    }
    else if (text[0] == 0x00) { // if RLE wasn't used
        auto temp = new uint8_t [size-1];
        for (uint32_t i=0; i < size-1; ++i) {
            temp[i] = text[i+1];
        }

        if (*aborting_var) return;

        std::swap(text, temp);

        delete[] temp;
        size--;
    }
    else throw std::invalid_argument("RLE was neither used nor not used, apparently");
}


void Compression::AC_make()
{
    if (*aborting_var) return;

    //  arithmetic coding - file size version
    //  based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)

    uint64_t whole = UINT_MAX;
    long double wholed = UINT_MAX;
    uint64_t half = roundl(wholed / 2.0);
    uint64_t quarter = roundl(wholed / 4.0);


    std::vector<uint64_t> r = model::AC::memoryless(text, size);

    if (*aborting_var) return;

    uint16_t r_size = 256;
    std::string output(4+4+r_size*4, 0);    // leaving 4 bits for compressed data size
    output.reserve(size);


    // saving original size on bits 4-7
    *(uint32_t*)(output.c_str()+4) = size;

    // cumulative mass functions
    std::vector<uint64_t> lower_bound(1, 0);  // vectors of lower and upper bounds for each char
    std::vector<uint64_t> upper_bound;

    uint8_t indexOfChars[256]={};   // given char, returns it'state index in lower_bound and upper_bound

    {
        uint16_t used_chars_ctr = 0;
        auto* output_arr_32b = (uint32_t*)(output.c_str()+8);
        for (uint16_t i = 0; i < r_size; i++) {
            // saving probabilities
            output_arr_32b[i] = r[i];

            if (r[i] != 0) {    // if character "i" exists in the model
                // add this char'state probabilities to CMFs
                lower_bound.emplace_back(lower_bound[used_chars_ctr] + r[i]);
                upper_bound.emplace_back(lower_bound[used_chars_ctr+1]);

                // adding char to index and incrementing the amount of used chars
                indexOfChars[i] = used_chars_ctr++;
            }
        }
    }

    //check if sum of probabilities represented as UINT_MAX is equal to whole
    assert(std::accumulate(r.begin(), r.end(), 0ull) == whole);

    //Actual encoding
    uint64_t low = 0;
    uint64_t high = whole;
    uint64_t width;
    uint32_t state = 0;

    TextWriteBitbuffer bitout(output);

    for (uint32_t i = 0; i < size and !*aborting_var; ++i)
    {
        width = high - low;
        high = low + roundl(((uint64_t)(upper_bound[indexOfChars[text[i]]] * width)) / wholed);
        low = low + roundl(((uint64_t)(lower_bound[indexOfChars[text[i]]] * width)) / wholed);

        assert(low < whole and high <= whole);
        assert(low < high);

        while (high < half or low >= half)
        {
            if (high < half)
            {
                bitout.add_bit_0();
                for (uint32_t j = 0; j < state; ++j) bitout.add_bit_1();

                state = 0;
                low *= 2;
                high *= 2;
            }
            else if (low >= half)
            {
                bitout.add_bit_1();
                for (uint32_t j = 0; j < state; ++j) bitout.add_bit_0();

                state = 0;
                low = 2 * (low - half);
                high = 2 * (high - half);
            }
        }

        while (low >= quarter and high < 3 * quarter)
        {
            state++;
            low = 2 * (low - quarter);
            high = 2 * (high - quarter);
        }
    }

    state++;
    if (low <= quarter)
    {
        bitout.add_bit_0();
        for (uint32_t j = 0; j < state; j++) bitout.add_bit_1();
    }
    else
    {
        bitout.add_bit_1();
        for (uint32_t j = 0; j < state; j++) bitout.add_bit_0();
    }

    bitout.flush();

    if (*aborting_var) return;

    // filling first 4 bits of output with compressed data size in   B I T S
    *(uint32_t*)(output.c_str()) = bitout.get_output_size();

    size = output.length();
    delete[] text;

    text = new uint8_t [size];
    for (uint32_t i=0; i < size; ++i) {
        text[i] = output[i];
    }
}


void Compression::AC_reverse()
{
    if (*aborting_var) return;

    uint16_t precision = 31;
    uint64_t whole = UINT_MAX;
    long double wholed = UINT_MAX;
    uint64_t half = roundl(wholed/2.0);
    uint64_t quarter = roundl(wholed/4.0);

    uint64_t sum = 0;
    std::vector<uint64_t> lower_bound(1, 0);
    std::vector<uint64_t> upper_bound;

    std::string alphabet;

    uint32_t compressed_size = *(uint32_t *)(text);
    uint32_t original_size = *(uint32_t *)(text + 4);

    uint16_t PMF_size = 256;
    auto* PMF = (uint32_t *)(text + 8);
    for ( uint16_t i=0; i < PMF_size; ++i)
    {
        if (PMF[i] != 0) {
            alphabet += char(i);
            sum += PMF[i];
            if (i != 255) lower_bound.push_back(sum);
            upper_bound.push_back(sum);
        }
    }

    TextReadBitbuffer in_bit(text, compressed_size, 4 + 4 + PMF_size * 4);

    uint64_t low = 0;
    uint64_t high = whole;
    uint64_t state = 0;
    uint64_t width;

    // loading initial state
    uint64_t i = 0;
    while (i <= precision and i < compressed_size)
    {
        if (in_bit.getbit()) state += (1ull << (precision - i)); //if bit's value == 1
        ++i;
    }

    std::string output;
    output.reserve(original_size);

    uint64_t new_low = 0;
    uint64_t new_high = 0;

    uint64_t output_size_counter = 0;

    // actual decoding
    while (!*aborting_var)
    {
        width = high - low;
        assert(state >= low and width <= whole );

        // predicted_value might be slightly wrong, because of rounding errors
        uint64_t predicted_value = floorl((long double)(state - low) * wholed / (long double)width);
        int16_t j = std::upper_bound(upper_bound.begin(), upper_bound.end(), predicted_value) - upper_bound.begin();

        new_high = low + roundl(upper_bound[j] * width / wholed);       // potential rounding errors

        // if the predicted position was indeed wrong, we'll correct that now
        if (new_high <= state) {
            j++;
            new_high = low + roundl(upper_bound[j] * width / wholed);   // potential rounding errors
        }
        new_low = low + roundl(lower_bound[j] * width / wholed);
        assert( j>=0 and j < alphabet.length() );
        assert(((new_low <= state) and (state < new_high)));
        assert(new_low < whole and new_high <= whole);
        assert(low <= whole);
        assert(high <= whole);
        assert(low < high);
        assert(new_low < new_high);

        output += alphabet[j];

        output_size_counter++;

        low = new_low;
        high = new_high;
        if (output_size_counter == original_size) {
            break;
        }

        while (high < half or low >= half)
        {
            if (high < half)
            {
                low <<= 1u;
                high <<= 1u;
                state <<= 1u;
            }
            else if (low >= half)
            {
                low = (low - half) << 1u;
                high = (high - half) << 1u;
                state = (state - half) << 1u;
            }

            if (i < compressed_size ) {
                if (in_bit.getbit()) state++;
                i++;
            }
        }
        while (low >= quarter and high < 3 * quarter)
        {
            low = (low - quarter) << 1u;
            high = (high - quarter) << 1u;
            state = (state - quarter) << 1u;
            if (i < compressed_size) {
                if (in_bit.getbit()) state++;
                i++;
            }
        }
    }

    if (*aborting_var) return;
    size = output.length();
    delete[] text;

    text = new uint8_t [size];
    for (uint32_t j=0; j < size; ++j) {
        text[j] = output[j];
    }
}


void Compression::AC2_make()
{
    //  arithmetic coding - file size version
    //  based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)

    if (*aborting_var) return;

    uint64_t whole = UINT_MAX;
    long double wholed = UINT_MAX;
    uint64_t half = roundl(wholed / 2.0);
    uint64_t quarter = roundl(wholed / 4.0);


    std::vector<std::vector<uint32_t>> rr = model::AC::order_1(text, size);

    if (*aborting_var) return;

    uint16_t r_size = 256;
    std::string output(4 + 4 + r_size * r_size * 4, 0);    // leaving 4 bits for compressed data size
    output.reserve(size + output.length());

    // saving original size on bits 4-7
    *(uint32_t*)(output.c_str()+4) = size;

    std::vector<std::vector<uint64_t>> lower_bound;
    std::vector<std::vector<uint64_t>> upper_bound;

    for (uint16_t r = 0; r < r_size; r++) {
        lower_bound.emplace_back(257);
        upper_bound.emplace_back(257);
        lower_bound[r][0] = 0;

        for (uint16_t i = 0; i < r_size; i++) {
            if (rr[r][i] != 0) {
                // saving probabilites to file, 256*256*4 bytes, unfortunately
                *(uint32_t*)(output.c_str()+(8 + r * 256 * 4 + 4 * i)) = rr[r][i];
            }
            // generating partial sums lower_bound and upper_bound
            lower_bound[r][i + 1] = (lower_bound[r][i] + rr[r][i]);
            upper_bound[r][i] = lower_bound[r][i + 1];
        }
        upper_bound[r][255] = whole;
    }

    //check if sum of probabilities represented as UINT_MAX is equal to whole
    if (true) {
        for (auto &r : rr) {
            assert(std::accumulate(r.begin(), r.end(), 0ull) == whole or std::accumulate(r.begin(), r.end(), 0ull) == 0);
        }
    }

    //Actual encoding
    uint64_t low = 0;
    uint64_t high = whole;
    uint64_t width;
    uint32_t state = 0;

    if (*aborting_var) return;

    TextWriteBitbuffer bitout(output);

    output += (char)text[0];  //  saving first char, for decoding purposes

    for (uint32_t i = 1; i < size and !*aborting_var; ++i) {
        width = high - low;
        high = low + roundl(((uint64_t)(upper_bound[text[i - 1]][text[i]] * width)) / wholed);
        low = low + roundl(((uint64_t)(lower_bound[text[i - 1]][text[i]] * width)) / wholed);

        assert(low < whole and high <= whole);
        assert(low < high);

        while (high < half or low >= half) {
            if (high < half) {
                bitout.add_bit_0();
                for (uint32_t j = 0; j < state; j++) {
                    bitout.add_bit_1();
                }
                state = 0;
                low *= 2;
                high *= 2;
            } else if (low >= half) {
                bitout.add_bit_1();
                for (uint32_t j = 0; j < state; j++) {
                    bitout.add_bit_0();
                }
                state = 0;
                low = 2 * (low - half);
                high = 2 * (high - half);
            }
        }
        while (low >= quarter and high < 3 * quarter) {
            state++;
            low = 2 * (low - quarter);
            high = 2 * (high - quarter);
        }
    }

    state++;
    if (low <= quarter) {
        bitout.add_bit_0();
        for (uint32_t j = 0; j < state; j++) {
            bitout.add_bit_1();
        }
    } else {
        bitout.add_bit_1();
        for (uint32_t j = 0; j < state; j++) {
            bitout.add_bit_0();
        }
    }

    bitout.flush();

    if (*aborting_var) return;

    // filling first 4 bits of output with compressed data size in   B I T S
    *(uint32_t*)(output.c_str()) = bitout.get_output_size();

    size = output.length();
    delete[] text;

    text = new uint8_t[size];
    for (uint32_t i = 0; i < size; ++i) {
        text[i] = output[i];
    }
}


void Compression::AC2_reverse()
{
    if (*aborting_var) return;
    uint16_t precision = 31;
    uint64_t whole = UINT_MAX;
    long double wholed = UINT_MAX;
    uint64_t half = roundl(wholed/2.0);
    uint64_t quarter = roundl(wholed/4.0);

    uint16_t r_size = 256;

    std::vector<std::vector<uint64_t>> lower_bound(256);
    std::vector<std::vector<uint64_t>> upper_bound(256);
    std::vector<std::string> alphabet(256);

    uint32_t compressed_size = *(uint32_t *)(text);
    uint32_t original_size = *(uint32_t *)(text + 4);

    for (uint16_t r = 0; r < r_size; ++r) {
        std::vector<uint64_t> temp_c(1,0);
        std::vector<uint64_t> temp_d;

        uint64_t sum = 0;
        uint8_t r_buffer[256 * 4];
        // loading probabilities into the buffer, for easier indexing
        for (uint16_t i = 0; i < r_size * 4; ++i) {
            r_buffer[i] = text[8 + r*r_size*4 + i];  // r array starts after 8 bytes
        }

        // calculating cumulative mass functions from these probabilities
        for (uint16_t i = 0; i < r_size; i++) {
            uint64_t rn = *(uint32_t*)(r_buffer+i*4);   // rn = PMF[i]

            if (rn != 0) {
                alphabet[r] += char(i);
                sum += rn;
                temp_d.push_back(sum);
                if (i!=255) {
                    temp_c.push_back(sum);
                }
            }
        }
        if (!temp_d.empty()) {
            lower_bound[r] = temp_c;
            upper_bound[r] = temp_d;
        }
    }

    if (*aborting_var) return;

    TextReadBitbuffer in_bit(text, compressed_size, 8 + 256 * 256 * 4 + 1);

    uint64_t low = 0;
    uint64_t high = whole;
    uint64_t state = 0;
    long double width;

    uint64_t i = 0;
    // loading initial state
    while ( i<=precision and i < compressed_size )
    {
        if (in_bit.getbit()) // if bit's value == 1
        {
            state += (1ull << (precision - i));
        }
        i++;
    }

    std::string output;

    uint64_t new_low;
    uint64_t new_high;

    uint8_t previous_char = text[8+r_size*r_size*4];    // first char of decoded text is normal ascii char
    output += previous_char;
    uint64_t output_size_counter = 1;

    // actual decoding
    while (output_size_counter != original_size and !*aborting_var)
    {
        assert(state <= high );
        width = high - low;
        assert(state >= low and width <= whole );

        // predicted_value might be slightly wrong, because of rounding errors
        uint64_t predicted_value = floorl((long double)(state - low) * wholed / (long double)width);
        int16_t j = std::upper_bound(upper_bound[previous_char].begin(), upper_bound[previous_char].end(), predicted_value ) - upper_bound[previous_char].begin();

        new_high = low + roundl(upper_bound[previous_char][j] * width / wholed);
        if (new_high <= state) {
            // if predicted_value was indeed wrong, we'll correct that now
            ++j;
            new_high = low + roundl(upper_bound[previous_char][j] * width / wholed);
        }
        new_low = low + roundl(lower_bound[previous_char][j] * width / wholed);

        assert( j>=0 and j < alphabet[previous_char].length() );
        assert(lower_bound[previous_char][j] < upper_bound[previous_char][j] );
        assert(((new_low <= state) and (state < new_high)));
        assert(new_low <= whole and new_high <= whole);
        assert(low <= whole);
        assert(high <= whole);
        assert(low < high);
        assert(new_low < new_high);

        output += alphabet[previous_char][j];
        previous_char = alphabet[previous_char][j];

        output_size_counter++;

        low = new_low;
        high = new_high;

        while (high < half or low >= half)
        {
            if (high < half)
            {
                low *= 2;
                high *= 2;
                state *= 2;
            }
            else if (low >= half)
            {
                low = (low - half) * 2;
                high = (high - half) * 2;
                state = (state - half) * 2;
            }

            if (i < compressed_size) {
                if (in_bit.getbit()) state++;
                ++i;
            }
            assert(state <= high);
        }
        while (low >= quarter and high < 3 * quarter)
        {
            low = (low - quarter) * 2;
            high = (high - quarter) * 2;
            state = (state - quarter) * 2;
            if (i < compressed_size) {
                if (in_bit.getbit()) state++;
                i++;
            }
            assert(state <= high);
        }
    }

    if (*aborting_var) return;

    size = output.length();
    delete[] text;

    text = new uint8_t [size];
    for (uint32_t j=0; j < size; ++j) {
        text[j] = output[j];
    }
}
