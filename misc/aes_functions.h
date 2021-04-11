//
// Created by pc on 11.04.2021.
//

#ifndef AES_FUNCTIONS_H
#define AES_FUNCTIONS_H

inline void increment_128b(__m128i& val, uint8_t val_arr[])
{
    for (int32_t i=15; i >= 0; --i)
    {
        ++val_arr[i];
        if (val_arr[i] != 0x00)
            break;
    }
    val = *((__m128i*) val_arr);
}

template <int round_counstant>
__m128i aes128_key_expansion(__m128i subkey) {
    // source: https://stackoverflow.com/questions/32297088/how-to-implement-aes128-encryption-decryption-using-aes-ni-instructions-and-gcc
    __m128i key_assisted = _mm_aeskeygenassist_si128(subkey, round_counstant);
    subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
    subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
    subkey = _mm_xor_si128(subkey, _mm_slli_si128(subkey, 4));
    key_assisted = _mm_shuffle_epi32(key_assisted, _MM_SHUFFLE(3, 3, 3, 3));
    return _mm_xor_si128(subkey, key_assisted);
}

void AES128_get_subkeys(uint8_t *key, __m128i *round_keys)
{
    round_keys[0] = _mm_loadu_si128((__m128i*) key);
    round_keys[1] = aes128_key_expansion<0x01>(round_keys[0]);
    round_keys[2] = aes128_key_expansion<0x02>(round_keys[1]);
    round_keys[3] = aes128_key_expansion<0x04>(round_keys[2]);
    round_keys[4] = aes128_key_expansion<0x08>(round_keys[3]);
    round_keys[5] = aes128_key_expansion<0x10>(round_keys[4]);
    round_keys[6] = aes128_key_expansion<0x20>(round_keys[5]);
    round_keys[7] = aes128_key_expansion<0x40>(round_keys[6]);
    round_keys[8] = aes128_key_expansion<0x80>(round_keys[7]);
    round_keys[9] = aes128_key_expansion<0x1B>(round_keys[8]);
    round_keys[10]= aes128_key_expansion<0x36>(round_keys[9]);
}


#endif //AES_FUNCTIONS_H
