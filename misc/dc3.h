#ifndef DC3_H
#define DC3_H

#include <cstdint>
#include <cmath>

namespace dc3
{
    /*
    template <class someInt>
    void print_array(someInt array[], uint64_t size, const std::string& message="", const std::string& sep=", ")
    {
        std::cout << message << '\n' << '[';
        for (uint64_t i=0; i < size; ++i)
        {
            std::cout << (uint64_t)array[i];
            if (i != size-1) std::cout << sep;
        }
        std::cout << ']' << std::endl;
    }//*/


    inline uint32_t get_B1(uint32_t i) { return 3*i + 1; }

    inline uint32_t get_B2(uint32_t i) { return 3*i + 2; }

    inline int32_t count_divisible(int64_t min, int64_t max, uint32_t div, uint32_t rem)
    {
        return ceil((max - rem) / div) - floor((min - rem) / div) + 1;
    }


    void counting_sort_indices(uint32_t*& indices, uint32_t tab[], uint32_t indicesSize, uint32_t maxTabVal, uint32_t offset)
    // Counting sort, which sorts indices[] by elements these indices point to in tab[]
    {
        uint32_t counters_size = maxTabVal + 1;
        auto counters = new uint32_t[counters_size]();
        auto sorted_indices = new uint32_t [indicesSize];

        // first we count occurences of given values:
        for (uint32_t i=0; i < indicesSize; ++i) {
            ++counters[tab[indices[i] + offset]];
        }

        // then we modify counters, so that output[counters[x]] is the place last occurrence of letter x should be:
        for (uint32_t i=1; i < counters_size; ++i)
            counters[i] += counters[i-1];

        // then we iterate over the indices array, and using counters the modified counters array,
        // we place the elements in sorted order:
        for (int64_t i = indicesSize - 1; i >= 0; --i) {
            sorted_indices[(counters[tab[indices[i]+offset]] -= 1)] = indices[i];
        }

        std::swap(sorted_indices, indices);
        delete[] sorted_indices;
        delete[] counters;
    }


    void DC3_recursion( uint32_t text[], uint32_t*& SA, uint64_t size, uint32_t max_val=256 )
    {
        // auto t0 = std::chrono::high_resolution_clock::now();

        uint32_t alphabet_size = max_val+1;
        auto alphabet = new uint32_t[alphabet_size]();

        alphabet[0] = 0;    // making sure the lowest element is in the alphabet, in case it's not in text


        uint32_t counter = 0;
        for (uint32_t i=0; i < size; ++i)
        {
            alphabet[text[i]] = text[i];
            counter++;
        }

        uint32_t current_letter = 1;
        for (uint32_t i=1; i < alphabet_size; ++i)
        {
            if (alphabet[i] != 0)
            {
                alphabet[i] = current_letter;
                current_letter++;
            }
        }
        uint32_t max_letter = current_letter;
        uint32_t translated_size = size;

        // padding translated with zeros, so that there are at least three of them at the end

        if ( text[size-1] != 0) translated_size += 3;
        else if (text[size-2] != 0) translated_size += 2;
        else if (text[size-3] != 0) translated_size += 1;


        auto translated = new uint32_t[translated_size]();

        // Translating the text to a shorter alphabet (for counting sort)
        for (uint32_t i=0; i < size; ++i) translated[i] = alphabet[text[i]];

        delete[] text;
        delete[] alphabet;
        // g_alphabet += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t0).count();


        // Calculating indices of every element not divisible by 3 for sorting
        uint32_t B1_offset = count_divisible(1, translated_size-3, 3, 1);
        uint32_t B12_size = B1_offset + count_divisible(2, translated_size-3, 3, 2);
        uint32_t B0_size = count_divisible(0, translated_size-3, 3, 0);

        auto B12_sorted = new uint32_t[B12_size];

        for (uint32_t i=0; i < B12_size; ++i) B12_sorted[i] = 0;

        uint32_t B1_max_index = ceil((double)B12_size/2);
        for (uint32_t i=0; i < B1_max_index; ++i)
        {
            B12_sorted[2*i] = 3*i+1;
        }

        uint32_t B2_max_index = ceil(((double)B12_size-1)/2);
        for (uint32_t i=0; i < B2_max_index; ++i)
        {
            B12_sorted[2*i+1] = 3*i+2;
        }


        uint32_t B12_max = B12_sorted[B12_size - 1];

        // t0 = std::chrono::high_resolution_clock::now();

        counting_sort_indices(B12_sorted, translated, B12_size, max_letter, 2);
        counting_sort_indices(B12_sorted, translated, B12_size, max_letter, 1);
        counting_sort_indices(B12_sorted, translated, B12_size, max_letter, 0);

        // g_counting_sort_1 += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t0).count();

        auto rank_array = new uint32_t [B12_size]();
        bool repeats = false;

        uint32_t current_rank = 1;  // rank 0 is reserved for [0,0,0], so we might as well skip it

        rank_array[0] = 1;

        // t0 = std::chrono::high_resolution_clock::now();
        for (uint32_t i=1; i < B12_size; ++i)
        {
            // looking for repeats within sorted text, they should obviously be next to each other
            if (not (translated[B12_sorted[i - 1]] == translated[B12_sorted[i]] and translated[B12_sorted[i - 1] + 1] == translated[B12_sorted[i] + 1] and translated[B12_sorted[i - 1] + 2] == translated[B12_sorted[i] + 2]))
                current_rank++;
            else
                repeats = true;
            rank_array[i] = current_rank;
        }

        auto translation_ranked = new uint32_t[translated_size]();

        for (uint32_t i=size; i < translated_size; ++i) translation_ranked[i] = 0;

        for (uint32_t i=0; i < B12_size; ++i) {
            translation_ranked[B12_sorted[i]] = rank_array[i];
        }
        delete[] rank_array;

        // g_ranking += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t0).count();

        if (repeats)    // 2 indices cannot have the same rank, so we need to address this issue
        {
            delete[] B12_sorted;    // B12_sorted will be overwriten later anyway

            uint32_t renamed_size = B12_size+1;
            auto renamed_with_ranks = new uint32_t [renamed_size]();

            for (uint32_t i=0; i < B1_offset; ++i)
            {
                renamed_with_ranks[i] = translation_ranked[get_B1(i)];
            }

            for (uint32_t i=0; i < B12_size-B1_offset; ++i)
            {
                renamed_with_ranks[i+B1_offset] = translation_ranked[get_B2(i)];
            }

            uint32_t sorted_ranks_size = renamed_size;
            uint32_t* sorted_ranks = nullptr;
            delete[] translation_ranked;



            DC3_recursion(renamed_with_ranks, sorted_ranks, sorted_ranks_size, current_rank);
            // std::cout << "Recursion complete!\n" << std::endl;

            // delete[] renamed_with_ranks;
            B12_sorted = new uint32_t [B12_size];

            for (uint32_t i=1; i < sorted_ranks_size; ++i)
            {
                if (sorted_ranks[i] < B1_offset)
                    B12_sorted[i-1] = get_B1(sorted_ranks[i]);
                else
                    B12_sorted[i-1] = get_B2(sorted_ranks[i]-B1_offset);
            }

            delete[] sorted_ranks;
            translation_ranked = new uint32_t [translated_size]();

            for (uint32_t i=0; i < B12_size; ++i) {
                translation_ranked[B12_sorted[i]] = i+1;
            }
        }

        // B0 - array of indices where every index % 3 == 0

        auto B0_sorted = new uint32_t [B0_size];
        for (uint32_t i=0; i < B0_size; ++i) B0_sorted[i] = i*3;

        uint32_t max_rank = size;

        // t0 = std::chrono::high_resolution_clock::now();
        // sort by rank of next suffix (which is always in B12, and we've sorted these already)
        counting_sort_indices(B0_sorted, translation_ranked, B0_size, size, 1);
        // sort by current letter
        counting_sort_indices(B0_sorted, translated, B0_size, max_letter, 0);

        // g_counting_sort_2 += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t0).count();

        // Both B0 and B12 are sorted now. Time to merge them into SA.
        // b0_i and b12_i will be our iterators for this process
        uint32_t b0_i = 0;
        uint32_t b12_i = 0;

        SA = new uint32_t [size+1];

        // t0 = std::chrono::high_resolution_clock::now();

        uint32_t i=0;
        for (; b0_i != B0_size and b12_i != B12_size; ++i) {

            if (B12_sorted[b12_i] % 3 == 1) {  // we know, that only 2 comparisons will be needed
                if (translated[B0_sorted[b0_i]] == translated[B12_sorted[b12_i]]) {  // comparing letters first

                    uint32_t B0_index = B0_sorted[b0_i] + 1;  // we know this guy is now in B12 array
                    uint32_t B12_index = B12_sorted[b12_i] + 1;  // same with this one

                    if (translation_ranked[B0_index] < translation_ranked[B12_index]) {
                        SA[i] = B0_sorted[b0_i++];
                    }
                    else {
                        SA[i] = B12_sorted[b12_i++];
                    }
                }
                else if (translated[B0_sorted[b0_i]] < translated[B12_sorted[b12_i]]) {
                    SA[i] = B0_sorted[b0_i++];
                }
                else {
                    SA[i] = B12_sorted[b12_i++];
                }
            }
            else {
                if (translated[B0_sorted[b0_i]] == translated[B12_sorted[b12_i]]) {  // comparing 1st letters
                    if (translated[B0_sorted[b0_i] + 1] == translated[B12_sorted[b12_i] + 1]) {  //comparing 2nd letters

                        uint32_t rank_B0 = translation_ranked[B0_sorted[b0_i] + 2];
                        uint32_t rank_B12 = translation_ranked[B12_sorted[b12_i] + 2];


                        if (rank_B0 < rank_B12) {
                            SA[i] = B0_sorted[b0_i++];
                        } else {
                            SA[i] = B12_sorted[b12_i++];
                        }
                    } else if (translated[B0_sorted[b0_i] + 1] < translated[B12_sorted[b12_i] + 1]) {
                        SA[i] = B0_sorted[b0_i++];
                    } else {
                        SA[i] = B12_sorted[b12_i++];
                    }
                } else if (translated[B0_sorted[b0_i]] < translated[B12_sorted[b12_i]]) {
                    SA[i] = B0_sorted[b0_i++];
                } else {
                    SA[i] = B12_sorted[b12_i++];
                }
            }
        }

        // appending the rest
        if (b0_i == B0_size) {
            for (uint32_t bi = b12_i; bi < B12_size;) {
                SA[i++] = B12_sorted[bi++];
            }
        }
        else if (b12_i == B12_size) {
            for (uint32_t bi=b0_i; bi < B0_size;) {
                SA[i++] = B0_sorted[bi++];
            }
        }

        // g_merging += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t0).count();

        delete[] translation_ranked;
        delete[] translated;
        delete[] B12_sorted;
        delete[] B0_sorted;
    }


    template<typename someInt>
    void DC3( someInt text[], uint32_t*& SA, uint64_t size, uint32_t max_val=0xFF )
    // General DC3 interface
    {
        // checks whether it makes any sense to even start this algorithm up
        switch (size)
        {
            case 0:
                return;

            case 1:
            {
                SA = new uint32_t [1];
                SA[0] = 0;
                return;
            }

            default:
            {
                uint32_t* proper_text = nullptr;
                // converting whatever datatype was used to 32-bit int
                proper_text = new uint32_t [size];
                for (uint32_t i=0; i < size; ++i) {
                    // conversion
                    proper_text[i] = text[i];

                    // making sure there are no 0s in the text, as we'll need a value lower than anything else later
                    ++proper_text[i];
                }


                // starting actual DC3
                DC3_recursion(proper_text, SA, size, max_val+1);


                // DC3 gives us suffix array with additional index for EOF we add during the algorithm, so let's filter it off
                auto SA2 = new uint32_t [size]();

                uint32_t SA2_i = 0;
                for (uint32_t SA_i=0; SA_i < size+1; ++SA_i) {
                    if (SA[SA_i] == size) SA_i++;

                    SA2[SA2_i] = SA[SA_i];
                    SA2_i++;
                }

                std::swap(SA, SA2);
                delete[] SA2;
                // proper_text will be deleted within DC3_recursion
            }
        }
    }


    template<typename someInt>
    void BWT_DC3( someInt text[], uint32_t*& SA, uint64_t size, uint32_t max_val=0xFF )
    // Interface specific to my implementation of BWT
    {
        // checks whether it makes any sense to even start this algorithm up
        switch (size)
        {
            case 0:
                return;

            case 1:
            {
                SA = new uint32_t [1];
                SA[0] = 0;
                return;
            }

            default:
            {
                // We'll need to convert whatever type text[] is to uint32_t for DC3 to work
                uint32_t new_size = size+2;
                auto* proper_text = new uint32_t [new_size];
                // making sure there are no 0s and 1s in the text, as we'll need a value lower than anything else later
                for (uint32_t i=0; i < size; ++i) proper_text[i] = text[i]+2;

                proper_text[new_size-2] = 1;    // our "EOF" symbol, needed for BWT
                proper_text[new_size-1] = 0;    // required by DC3_recursion


                // starting actual DC3
                DC3_recursion(proper_text, SA, new_size, max_val+2);

                // DC3 gives us suffix array with additional index for EOF we add during the algorithm, so let's filter it off
                auto SA2 = new uint32_t [size+1]();

                uint32_t SA2_i = 0;
                for (uint32_t SA_i=0; SA_i < new_size; ++SA_i) {
                    if (SA[SA_i] == size+1) continue;   // this one is pointing outside original text

                    SA2[SA2_i] = SA[SA_i];
                    SA2_i++;
                }

                std::swap(SA, SA2);
                delete[] SA2;
                // proper_text will be deleted within DC3_recursion
            }
        }
    }
}

#endif // DC3_H
