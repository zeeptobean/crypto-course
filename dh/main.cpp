#include <iostream>
#include <cassert>
#include "prng.hpp"
#include "uint512.hpp"

uint512_t modular_exponentiation(uint512_t base, uint512_t exponent, const uint512_t& mod) {
    // std::cout << "Calculating modular exponentiation..." << std::endl;
    // std::cout << "Base: " << to_string_decimal_uint512(base) << std::endl;
    // std::cout << "Exponent: " << to_string_decimal_uint512(exponent) << std::endl;
    // std::cout << "Modulus: " << to_string_decimal_uint512(mod) << std::endl;
    uint512_t result = 1;
    base = base % mod;
    for(int i = 0; i < 512; i++) {
        if(getbit_uint512(exponent, i)) {
            result = mul_mod_uint512(result, base, mod);
        }
        base = mul_mod_uint512(base, base, mod);
    }
    return result;
}

uint512_t generate512_uint512(PRNG& prng) {
    uint512_t result;
    for(int i = 0; i < 8; i++) {
        result.v[i] = prng.next_u64();
    }
    return result;
}

uint512_t generate256_uint512(PRNG& prng) {
    uint512_t result;
    for(int i = 0; i < 4; i++) {
        result.v[i] = prng.next_u64();
    }
    return result;
}

uint512_t generate128_uint512(PRNG& prng) {
    uint512_t result;
    for(int i = 0; i < 2; i++) {
        result.v[i] = prng.next_u64();
    }
    return result;
}

uint512_t generate_odd_uint512(PRNG& prng, size_t bit_length = 512) {
    assert(bit_length <= 512);
    uint512_t result;
    for(int i = 0; i < 8; i++) {
        result.v[i] = prng.next_u64();
    }
    size_t word_index = (bit_length-1) / 64;
    size_t bit_index = (bit_length-1) % 64;
    for(size_t i = word_index + 1; i < 8; i++) {
        result.v[i] = 0;
    }
    if (bit_index < 63) result.v[word_index] &= (1ULL << (bit_index + 1)) - 1;
    result.v[word_index] |= (1ULL << bit_index);    // force exact bit length
    result.v[0] |= 1;                               // odd

    return result;
}

bool rabin_miller_test(PRNG& prng, const uint512_t& n, size_t k = 3) {
    // filter small primes
    const uint16_t small_primes[12] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for(size_t i = 0; i < 12; i++) {
        if(n == small_primes[i]) return true;
        uint64_t rem;
        div64_uint512(n, small_primes[i], rem);
        if(rem == 0) return false;
    }

    uint512_t m = n - 1;
    uint512_t d = m;
    size_t s = 0;
    while(getbit_uint512(d, 0) == 0) {
        d = shift_right_uint512(d, 1);
        s++;
    }

    for(size_t i = 0; i < k; i++) {
        uint512_t a = generate256_uint512(prng);
        a = mod_uint512(a, n - 4) + 2; // random in [2, n-2]
        // case 1: little fermat
        uint512_t x = modular_exponentiation(a, d, n);
        if(x == 1 || x == m) continue;

        // case 2: test
        bool composite = true;
        for(size_t j = 0; j < s; j++) {
            x = mul_mod_uint512(x, x, n);
            if(x == m) {
                composite = false;
                break;
            }
        }
        if(composite) {
            return false;
        }
    }
    return true;
}

uint512_t generate_safe_prime(PRNG& prng, size_t bit_length = 512) {
    assert(bit_length <= 512);
    while(true) {
        uint512_t q = generate_odd_uint512(prng, bit_length);
        std::cout << "Generating q: " << to_string_decimal_uint512(q) << std::endl; 
        if(!rabin_miller_test(prng, q)) {
            continue;
        }
        // uint512_t p = (q - 1) >> 1;
        // std::cout << "Generating p: " << to_string_decimal_uint512(p) << std::endl; 
        // if(!rabin_miller_test(prng, p)) {
        //     continue;
        // }
        return q;
    }
}

int main() {
    PRNG prng(777);
    uint512_t num = generate_safe_prime(prng, 256);
    std::cout << "A prime number: " << num << std::endl;
    std::cout << "In decimal: " << to_string_decimal_uint512(num) << std::endl;
    return 0;
}