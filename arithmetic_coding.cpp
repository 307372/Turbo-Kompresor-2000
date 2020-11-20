#include "arithmetic_coding.h"


arithmetic_coding::arithmetic_coding()
{

    std::numeric_limits<long double> test_long_double;
    if (test_long_double.is_specialized) {
        if ( test_long_double.is_iec559 != 1 )          throw VariableTypeException();
        if ( test_long_double.digits != 64 )            throw VariableTypeException();
        if (test_long_double.max_exponent10 != 4932)    throw VariableTypeException();
    } else {
        throw VariableTypeException( "long double is not specialized in <numeric_limits>. It probably means the algorithm won't work on your computer." );
    }

    precision = 31;
    whole = UINT_MAX;
    wholed = UINT_MAX;
    half = roundl(wholed/2.0);
    quarter = roundl(wholed/4.0);



}


uint64_t arithmetic_coding::encode_file_to_fstream( std::fstream &dst_stream, std::string path_to_target_file, size_t buffer_size )
{

    /*arithmetic coding - file size version
    based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)
    X - alphabet
    r = p * whole, where p = probability mass function on X
    x - message
    */

    statistical_tools st( path_to_target_file );
    st.iid_model_chunksV2();
    st.target_file.close();

    uint16_t r_size = 256;


    auto probabilities_buffer = new uint8_t[r_size * 4];
    for (uint16_t i=0; i < r_size; i++) {
        probabilities_buffer[4 * i] = st.r[i] & 0xFFu;
        probabilities_buffer[4 * i + 1] = (st.r[i] >> 8u) & 0xFFu;
        probabilities_buffer[4 * i + 2] = (st.r[i] >> 16u) & 0xFFu;
        probabilities_buffer[4 * i + 3] = (st.r[i] >> 24u) & 0xFFu;
    }


    dst_stream.write((char*)probabilities_buffer, r_size * 4 );
    delete[] probabilities_buffer;

    output_bitbuffer bitout(dst_stream);




    std::vector<uint64_t> c;
    std::vector<uint64_t> d;
    for (uint16_t i=0; i < r_size; i++) // generate c and d, where c[i] is lower bound to get i-th letter of our alphabet, and d[i] is the upper bound
    {
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
        //std::cout<<sum<<std::endl;
        assert(sum == whole);
    }

    //Actual encoding
    uint64_t a = 0;
    uint64_t b = whole;
    //std::string wynik = "";
    uint32_t s = 0;

    std::ifstream target_file;
    target_file.open( path_to_target_file );
    std::vector<char> buffer (buffer_size, 0);

    while (!target_file.eof())
    {
        target_file.read(buffer.data(), buffer.size());
        std::streamsize dataSize = target_file.gcount();
        for (auto it = buffer.begin(); it != buffer.begin() + dataSize; ++it)
        {

            uint64_t w = b-a;

            b = a + roundl(((uint64_t)(d[(unsigned char)(*it)]*w))/wholed); //potential rounding errors

            a = a + roundl(((uint64_t)(c[(unsigned char)(*it)]*w))/wholed);
            assert( a<whole and b<whole );
            assert( a<b );
            while (b < half or a > half)
            {
                if (b < half)
                {
                    //wynik += "0";
                    bitout.add_bit_0();
                    for (uint32_t j = 0; j < s; j++) {
                        //wynik += "1";
                        bitout.add_bit_1();
                    }
                    s = 0;
                    a *= 2;
                    b *= 2;
                }
                else if (a > half)
                {

                    //wynik += "1";
                    bitout.add_bit_1();
                    for (uint32_t j = 0; j < s; j++) {
                        //wynik += "0";
                        bitout.add_bit_0();
                    }
                    s = 0;
                    a = 2 * (a-half);
                    b = 2 * (b-half);
                }
            }
            while (a > quarter and b < 3*quarter)
            {
                s++;
                a = 2 * (a-quarter);
                b = 2 * (b-quarter);
            }
        }
    }
    target_file.close();

    //for (size_t i=0; i < x.size(); i++)

    s++;
    if (a <= quarter)
    {
        //wynik += "0";
        bitout.add_bit_0();
        for (uint32_t j = 0; j < s; j++) {
            //wynik += "1";
            bitout.add_bit_1();
        }
    }
    else
    {
        //wynik += "1";
        bitout.add_bit_1();
        for (uint32_t j = 0; j < s; j++) {
            //wynik += "0";
            bitout.add_bit_0();
        }
    }

    bitout.flush();
    //std::cout << wynik << std::endl;
    return bitout.get_output_size();    // returns size of outputted data in   B I T S

};


void arithmetic_coding::decode_to_file_from_fstream(std::fstream &source, std::string path_for_output, uint64_t compressed_file_size, uint64_t uncompressed_file_size )
{
    auto t0 = std::chrono::high_resolution_clock::now();
    uint16_t r_size = 256;

    assert( source.is_open() );
    uint64_t sum = 0;
    std::vector<uint64_t> c;
    std::vector<uint64_t> d;
    c.push_back(0);
    std::string alphabet;

    // uint64_t compressed_data_length = (compressed_file_size - r_size*4)*8-2; // Needs to be abstract

    auto r_buffer = new uint8_t[r_size * 4];
    source.read((char*)r_buffer, r_size * 4 );
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
            c.push_back(sum);
            d.push_back(sum);
        }
    }
    d.push_back( whole );
    delete[] r_buffer;


    auto t1 = std::chrono::high_resolution_clock::now();

    input_bitbuffer in_bit(source, compressed_file_size ); // size of actual data excluding array of r's prepended to it
    /*
    std::cout << "inbit:\n";
    for (int i=0; i < compressed_data_length; i++ ) std::cout << in_bit.getbit();
    std::cout << std::endl;
    */

    uint64_t a = 0;
    uint64_t b = whole;
    uint64_t z = 0;
    uint64_t w = 0;


    // actual decoding

    uint64_t i = 0;
    while ( i<=precision and i < compressed_file_size )
    {

        if (in_bit.getbit()) //if bit's value == 1
            z += (1ull<<(precision-i));
        i++;
    }

    std::ofstream output(path_for_output);
    assert (output.is_open());

    uint64_t a0 = 0;
    uint64_t b0 = 0;
    //bool EOF_found = false;
    //std::string wynik = "";
    uint64_t output_size_counter = 0;

    // Timing
    //std::vector<std::chrono::microseconds> durations_for;
    //std::vector<std::chrono::microseconds> durations_while;
    //auto t2 = std::chrono::high_resolution_clock::now();

    while (true)
    {
        //t0 = std::chrono::high_resolution_clock::now();

        w = b - a;
        uint16_t j = std::distance(c.begin(), std::lower_bound(c.begin(), c.end(), (z - a) * wholed / w));

        b0 = a + roundl(d[j] * w / wholed); //potential rounding errors
        a0 = a + roundl(c[j] * w / wholed);
        if (a0 > z) {

            j--;
            b0 = a + roundl(d[j] * w / wholed); //potential rounding errors
            a0 = a + roundl(c[j] * w / wholed);
        }
        if (b0 < z) {
            std::cout << "powiekszono\n";
            j++;
            b0 = a + roundl(d[j] * w / wholed); //potential rounding errors
            a0 = a + roundl(c[j] * w / wholed);
        }

        assert(((a0 <= z) and (z <= b0)));
        assert(a0 <= whole and b0 <= whole);
        assert(a <= whole);
        assert(b <= whole);
        assert(a < b);
        assert(a0 < b0);

        output << alphabet[j];

        output_size_counter++;

        a = a0;
        b = b0;
        if (output_size_counter == uncompressed_file_size) {
            //EOF_found = true;
            break;
        }

        //t1 = std::chrono::high_resolution_clock::now();
        while (b < half or a > half)
        {
            if (b < half)
            {
                a <<= 1u;
                b <<= 1u;
                z <<= 1u;
            }
            else if (a > half)
            {
                a = (a-half)<<1u;
                b = (b-half)<<1u;
                z = (z-half)<<1u;
            }

            if (i < compressed_file_size )
                if ( in_bit.getbit() ) z++;
            i++;
        }
        while (a > quarter and b < 3*quarter)
        {
            a = (a-quarter)<<1u;
            b = (b-quarter)<<1u;
            z = (z-quarter)<<1u;
            if (i < compressed_file_size)
                if ( in_bit.getbit() ) z++;
            i++;
        }
        //t2 = std::chrono::high_resolution_clock::now();

        //durations_while.push_back( std::chrono::duration_cast<std::chrono::microseconds>(t2-t1) );
        //durations_for.push_back( std::chrono::duration_cast<std::chrono::microseconds>(t1-t0) );
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    //std::chrono::microseconds while_time;
    //for (auto wi : durations_while) while_time += wi;

    //std::chrono::microseconds for_time;
    //for (auto fi : durations_for) for_time += fi;

    //std::cout << "for loops took:\t" << for_time.count() << std::endl;
    //std::cout << "while loops took:\t" << while_time.count() << std::endl;

    std::cout << "Preparations:\t" << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << " ms\n";
    std::cout << "Decompression:\t" << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " ms" << std::endl;
    // std::cout << wynik << std::endl;
    output.close();
};



output_bitbuffer::output_bitbuffer(std::fstream &os) {
    this->os = &os;
    sum_of_outputted_bits=0;
    clear();
}

output_bitbuffer::~output_bitbuffer() {
    if ((bi != 0) or (bitcounter != 0)) flush();
}

void output_bitbuffer::clear() {
    for (auto &i : buffer) i = 0;
    bi = 0;
    bitcounter = 0;
}

void output_bitbuffer::flush() {
    if (bitcounter > 0)
        os->write( (char*)buffer, bi+1 );
    else
        os->write( (char*)buffer, bi );
    clear();
}

void output_bitbuffer::add_bit_1() {
    buffer[bi] <<= 1u;
    ++buffer[bi];
    bitcounter++;
    sum_of_outputted_bits++;
    if ( bitcounter == 8 ) {
        bi++;
        bitcounter = 0;
        if (bi >= 8*1024) flush();
    }
}

void output_bitbuffer::add_bit_0() {
    buffer[bi] <<= 1u;
    sum_of_outputted_bits++;
    bitcounter++;
    if ( bitcounter == 8 ) {
        bi++;
        bitcounter = 0;
        if (bi >= 8*1024) flush();
    }
}

uint64_t output_bitbuffer::get_output_size() const {
    return sum_of_outputted_bits;
}

// input_bitbuffer

input_bitbuffer::input_bitbuffer(std::fstream &os, uint64_t compressed_size) {
    this->os = &os;
    this->data_left_b = compressed_size;    // shouldn't work
    this->buffer_size = 8*1024;
    this->output_bit = false;
    for (auto &i : this->buffer) i = 0;
    this->bits_left_in_buffer = 0;
    this->meaningful_bits = 8;
    this->bi = 0;
    this->bitcounter = 0;
    this->fill_buffer();
}


void input_bitbuffer::fill_buffer() {
    if (data_left_b != 0) {
        if (data_left_b < 8) {
            os->read((char *) buffer, 1);
            meaningful_bits = data_left_b;
            bits_left_in_buffer = data_left_b;
            data_left_b = 0;
        }

        else if (data_left_b >= buffer_size * 8) {
            os->read((char *) buffer, buffer_size);
            bits_left_in_buffer = buffer_size*8;
            data_left_b -= buffer_size*8;
        } else {
            if (data_left_b % 8 == 0) {
                os->read((char *) buffer, data_left_b / 8);
                bits_left_in_buffer = data_left_b;
                data_left_b -= data_left_b;
            } else {
                os->read((char *) buffer, data_left_b / 8);
                bits_left_in_buffer = (data_left_b/8)*8;
                data_left_b -= (data_left_b/8)*8;
            }
        }
        bi = 0;
        bitcounter = 0;
    }
    //else throw NothingLeftToReadException();

}


bool input_bitbuffer::getbit() {
    output_bit = (buffer[bi] >> (meaningful_bits-bitcounter-1)) & 1u; // not finished yet
    bitcounter++;
    bits_left_in_buffer--;
    if(bitcounter == 8 or bits_left_in_buffer == 0) {

        if (bits_left_in_buffer == 0) fill_buffer();
        else {  // bit_counter == 8, so:
            bi++;
            bitcounter = 0;
        }
    }
    return output_bit; // (buffer[bi] >> bitcounter-1) & 1u;
}

















