#ifndef MODELS_H
#define MODELS_H


#include <climits>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <vector>

namespace model {

    void normalize_frequencies(std::vector<uint64_t>& freq, uint64_t upper_limit, uint64_t sum_of_freq=0) {
        if (sum_of_freq == 0)
            sum_of_freq = std::accumulate(std::begin(freq), std::end(freq), 0ull);

        if (sum_of_freq == upper_limit) {
            // making it so that sum_of_freq == upper_limit is the point of this function
            return;
        }
        for (unsigned long & counter : freq) {
            counter *= upper_limit;
            counter /= sum_of_freq;
        }

        // probabilities must sum to 1   =>   counters must sum to upper limit
        uint64_t rsum2 = std::accumulate(std::begin(freq), std::end(freq), 0ull);
        if (rsum2 > upper_limit) {
            *std::max_element(std::begin(freq), std::end(freq)) -= rsum2 - upper_limit;
        } else if (rsum2 < upper_limit) {
            *std::max_element(std::begin(freq), std::end(freq)) += upper_limit - rsum2;
        }
        assert(std::accumulate(freq.begin(), freq.end(), 0ull) == upper_limit);
    }

    std::vector<uint64_t> count_chars(uint8_t text[], uint64_t text_size) {
        std::vector<uint64_t> counters(256, 0);
        for (uint32_t i = 0; i < text_size; ++i) {
            ++counters[text[i]];
        }
        return counters;
    }

    namespace AC    // Arithmetic Coding
    {
        std::vector<std::vector<uint32_t>> order_1(uint8_t text[], uint32_t text_size) {
            uint64_t r[256];
            std::vector<std::vector<uint32_t>> rr(256, std::vector<uint32_t>(256, 0));

            uint64_t max = UINT32_MAX;
            for (uint16_t i = 0; i < 256; i++) for (uint16_t j = 0; j < 256; j++) rr[i][j] = 0;   // zeroing rr
            for (auto &i : r) i = 0;   // zeroing r

            uint8_t previous_char = text[0];
            for (uint32_t i = 1; i < text_size; ++i) {
                rr[previous_char][text[i]]++;
                r[previous_char]++;
                previous_char = text[i];
            }

            // scaling every tab, so that sum of their elements = this->max
            for (uint16_t i = 0; i < 256; ++i)
                for (uint16_t j = 0; j < 256; ++j) {
                    bool less_than_one = false;
                    if ((long double) rr[i][j] * (long double) max / (long double) r[i] < 1 and
                        rr[i][j] != 0)
                        less_than_one = true;
                    rr[i][j] = (uint32_t) (roundl((long double) rr[i][j] * (long double) max / (long double) r[i]));
                    if (less_than_one) assert(rr[i][j] != 0);
                }

            // compensating for rounding errors
            for (uint16_t i = 0; i < 256; i++) {
                std::vector<uint32_t> *current_r = &rr[i];
                int64_t diff = (int64_t) UINT32_MAX -
                               (int64_t) std::accumulate(std::begin(*current_r), std::end(*current_r), 0ull);
                if (diff == UINT32_MAX or diff == 0)
                    continue;  // if vector is empty or sum of its element is exacly what we want, continue
                else if (diff < 0) {

                    while (diff != 0) {
                        for (uint32_t ch = 0; ch < 256 and diff != 0; ++ch) {
                            if ((*current_r)[ch] > std::abs(diff) and (*current_r)[ch] != 0) {
                                (*current_r)[ch]--;
                                diff++;
                            }
                        }
                    }
                    diff = (int64_t) UINT32_MAX -
                           (int64_t) std::accumulate(std::begin(*current_r), std::end(*current_r), 0ull);
                } else if (diff > 0) {
                    while (diff != 0)
                        for (uint32_t ch = 255; ch >= 0 and diff != 0; --ch)
                            if ((*current_r)[ch] > std::abs(diff) and (*current_r)[ch] != 0) {
                                (*current_r)[ch]++;
                                diff--;
                            }
                    diff = (int64_t) UINT32_MAX -
                           (int64_t) std::accumulate(std::begin(*current_r), std::end(*current_r), 0ull);
                }

                assert(diff == 0 or diff == UINT32_MAX);
            }
            return rr;
        }

        std::vector<uint64_t> memoryless(uint8_t text[], uint32_t text_size) {
            assert(text_size != 0);
            uint64_t max = UINT32_MAX;

            std::vector<uint64_t> r = count_chars(text, text_size);

            normalize_frequencies(r, max, text_size);

            assert(std::accumulate(r.begin(), r.end(), 0ull) == max);
            return r;
        }
    }
    namespace ANS   // Asymmetric Numeral Systems
    {
        std::vector<uint64_t> memoryless( uint8_t text[], uint32_t text_size )
        {
            assert(text_size != 0);
            uint64_t max = 1l<<32;

            std::vector<uint64_t> r(256,0);

            for (uint32_t i=0; i < text_size; ++i) {
                r[text[i]]++;
            }

            normalize_frequencies(r, max);

            return r;
        }

    }
}


#endif // MODELS_H
