#include "statistical_tools.h"
#include <fstream>
#include <iostream>
#include <map>
#include <climits>
#include <assert.h>
#include <cmath>
#include <iterator>
#include <chrono>

statistical_tools::statistical_tools( std::string absolute_path_to_file )
{
    this->r = {};
    this->alphabet = "";
    this->target_file.open(absolute_path_to_file);
    this->max = UINT_MAX;
}

statistical_tools::~statistical_tools()
{
    if (this->target_file.is_open())
        this->target_file.close();
}

void statistical_tools::iid_model()
{
    //Produces statistical data compatible with my arithmetic coding implementation
    std::map<char, unsigned long> counter = {};
    if (this->target_file.is_open())
    {
        auto start = std::chrono::high_resolution_clock::now();

        long double sum = 0;
        char ch;
        while (this->target_file >> std::noskipws >> ch) //read file char by char
        {
            counter[ch]++;
            sum++;
        }

        auto stop = std::chrono::high_resolution_clock::now();

        unsigned long long rsum = 0;
        unsigned long long rn = 0;
        for (auto& i : counter)
        {
            this->alphabet += i.first;
            rn = (this->max/sum)*i.second;
            if (not(rsum <= (this->max-rn))) // guard against sum of r being over this->max
            {
                rn = this->max-rsum;
            }
            rsum += rn;
            this->r.push_back( rn );
        }
        if (rsum != this->max) // guard against sum of r being under this->max
        {
            this->r.back() += this->max - rsum;
            rsum += this->max - rsum;
        }

        assert(rsum == this->max);

        for (auto& i : counter)
            std::cout << i.first << "   " << i.second << "\n";
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( stop - start );
        std::cout << std::endl << "duration: " << duration.count();

    }
}

void statistical_tools::iid_model_chunks( size_t buffer_size )
{
    //Produces statistical data compatible with my arithmetic coding implementation.
    //It reads the file in parts until EOF.
    std::map<char, unsigned long long> counter = {};
    if (this->target_file.is_open())
    {
        long double sum = 0;
        std::vector<char> buffer (buffer_size, 0); //reads only 1024 bytes at a time
        while (!this->target_file.eof())
        {
            this->target_file.read(buffer.data(), buffer.size());
            std::streamsize dataSize = this->target_file.gcount();
            for (auto it = buffer.begin(); it != buffer.begin() + dataSize; ++it)
            {
                counter[*it]++;
                sum++;
            }
        }



        //auto stop = std::chrono::high_resolution_clock::now();
        this->file_size = sum; // file size in bytes

        unsigned long long rsum = 0;
        unsigned long long rn = 0;
        for (auto& i : counter)
        {
            this->alphabet += i.first;
            rn = (this->max/sum)*i.second;
            if (not(rsum <= (this->max-rn))) // guard against sum of r being over this->max
            {
                rn = this->max-rsum;
            }
            rsum += rn;
            this->r.push_back( rn );
        }
        if (rsum != this->max) // guard against sum of r being under this->max
        {
            this->r.back() += this->max - rsum;
            rsum += this->max - rsum;
        }

        assert(rsum == this->max);

    }
    else
        assert( this->target_file.is_open() );
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

























