#include "arithmetic_coding.h"
#include <climits>
#include <vector>
#include <string>
#include <assert.h>
#include <algorithm>
#include <map>
#include <string>
#include <iostream>
#include <cmath>
#include <fstream>

unsigned long long ullround(long double ld)
{

    if (ld>LLONG_MAX)
    {
        //std::cout<< "1#" << ld << " => " << (unsigned long long)ld << "     R=" << ld << std::endl;
        return (unsigned long long)ld;
    }
    else
    {
        //std::cout<< "2#" << ld << " => " << (unsigned long long)llround(ld) << std::endl;
        return  (unsigned long long)llround(ld);
    }
}

arithmetic_coding::arithmetic_coding()
{
    precision = 31;
    whole = UINT_MAX;
    wholed = UINT_MAX;
    half = ullround(wholed/2.0);
    quarter = ullround(wholed/4.0);
}

std::string arithmetic_coding::encode_file( std::string X, std::vector<unsigned long> r, std::string path, size_t buffer_size )
{

  /*arithmetic coding - file size version
    based on algorithm from youtube (Information Theory playlist by mathematicalmonk, IC 5)
    X - alphabet
    p - probability mass function on X
    x - message
    EOF_symbol - signals end of some encoded sequence
    */

    std::vector<unsigned long> c;
    std::vector<unsigned long> d;
    for (size_t i=0; i < r.size(); i++) // generate c and d, where c[i] is lower bound to get i-th letter of alphabet, and d[i] is the upper bound
    {
        unsigned long sum = 0;
        std::for_each(r.begin(), r.begin()+i, [&] (unsigned long j) {sum += j;} );
        c.push_back(sum);
        if (i != r.size()-1)
            d.push_back(sum+r[i]);
        else
            d.push_back(whole);
    }


    //check if sum of probabilities represented as UINT_MAX is equal to whole
    if (true)
    {
        unsigned long sum = 0;
        for (auto& n : r)
            sum += n;
        //std::cout<<sum<<std::endl;
        assert(sum == whole);
    }

    std::map<char, int> symbol2index;
    for (size_t i=0; i < X.size(); i++)
        symbol2index[X[i]]=i;

    //Actual encoding
    unsigned long a = 0;
    unsigned long b = whole;
    std::string wynik = "";
    int s = 0;

    std::ifstream target_file;
    target_file.open(path);
    std::vector<char> buffer (buffer_size, 0);

    while (!target_file.eof())
    {
        target_file.read(buffer.data(), buffer.size());
        std::streamsize dataSize = target_file.gcount();
        for (auto it = buffer.begin(); it != buffer.begin() + dataSize; ++it)
        {

            unsigned long w = b-a;

            b = a + ullround(((long double)(d[symbol2index[*it]]*w))/wholed); //potential rounding errors

            a = a + ullround(((long double)(c[symbol2index[*it]]*w))/wholed);
            assert( a<whole and b<whole );
            assert( a<b );
            while (b < half or a > half)
            {
                if (b < half)
                {
                    wynik += "0";
                    for (int j = 0; j < s; j++)
                        wynik += "1";
                    s = 0;
                    a *= 2;
                    b *= 2;
                }
                else if (a > half)
                {
                    wynik += "1";
                    for (int j = 0; j < s; j++)
                        wynik += "0";
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
        wynik += "0";
        for (int j = 0; j < s; j++)
            wynik += "1";
    }
    else
    {
        wynik += "1";
        for (int j = 0; j < s; j++)
            wynik += "0";
    }
    return wynik;
};

void arithmetic_coding::decode_to_file(std::string encoded_message, std::vector<unsigned long> r, std::string alphabet, std::string path, unsigned long long file_size)
{
    unsigned long a = 0;
    unsigned long b = whole;
    unsigned long z = 0;
    unsigned long w = 0;


    std::vector<unsigned long> c;
    std::vector<unsigned long> d;
    for (size_t j=0; j < r.size(); j++)
    {
        unsigned long sum = 0;
        std::for_each(r.begin(), r.begin()+j, [&] (unsigned long k) {sum += k;} );
        c.push_back(sum);
        if (j != r.size()-1)
            d.push_back(sum+r[j]);
        else
            d.push_back(whole);
    }

    std::map<char, int> symbol2index;
    for (size_t j=0; j < alphabet.size(); j++)
        symbol2index[alphabet[j]]=j;

    int i = 0;
    while ( i<=precision and (size_t)i < encoded_message.size() )
    {
        if (encoded_message[i] == '1')
            z += (1ull<<(precision-i));
        i++;
    }

    std::ofstream output;
    output.open(path);
    unsigned long a0 = 0;
    unsigned long b0 = 0;
    bool EOF_found = false;
    std::string wynik = "";
    unsigned long long output_size_counter = 0;
    while (!EOF_found)
    {
        for (int j=0; (size_t)j < alphabet.size(); j++)
        {
            w = b-a;
            b0 = a + ullround(((long double)(d[j]*w))/wholed); //potential rounding errors
            a0 = a + ullround(((long double)(c[j]*w))/wholed);
            assert( a0<=whole and b0<=whole );
            assert( a<=whole );
            assert( b<=whole );
            assert( a<b );
            assert( a0<b0 );
            if ((a0 <= z) and (z < b0))
            {

                output << alphabet[j];
                wynik += alphabet[j];
                output_size_counter++;
                //std::cout<<alphabet[j];
                a = a0;
                b = b0;
                if ( output_size_counter == file_size )
                {
                    EOF_found = true;
                    break;
                }
                break;
            }
        }
        while ((b < half or a > half) and !EOF_found)
        {
            if (b < half)
            {
                a *= 2;
                b *= 2;
                z *= 2;
            }
            else if (a > half)
            {
                a = 2*(a-half);
                b = 2*(b-half);
                z = 2*(z-half);
            }

            if ((size_t)i < encoded_message.size() and encoded_message[i] == '1')
                z++;
            i++;
        }
        while ((a > quarter and b < 3*quarter) and !EOF_found)
        {
            a = 2*(a-quarter);
            b = 2*(b-quarter);
            z = 2*(z-quarter);
            if ((size_t)i < encoded_message.size() and encoded_message[i] == '1')
                z++;
            i++;
        }
    }
    output.close();
};

















