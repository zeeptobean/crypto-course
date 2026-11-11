#include "prng.hpp"
#include "uint512.hpp"
#include <cassert>
#include <iostream>

uint512_t modular_exponentiation(uint512_t base, uint512_t exponent, const uint512_t& mod) {
    uint512_t result = 1;
    base = mod_uint512(base, mod);
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

uint512_t generate_odd_uint512(PRNG& prng, size_t bit_length = 512) {
    assert(bit_length <= 512);
    uint512_t result;
    for(int i = 0; i < 8; i++) {
        result.v[i] = prng.next_u64();
    }
    result.v[0] |= 1;   // odd
    // mask the bits above bit_length
    size_t word_index = (bit_length-1) / 64;
    size_t bit_index = (bit_length-1) % 64;
    for(size_t i = word_index + 1; i < 8; i++) {
        result.v[i] = 0;
    }
    if(bit_length < 512) {
        result.v[word_index] &= (1ULL << (bit_index + 1)) - 1;
    } 

    return result;
}

bool rabin_miller_test(const uint512_t& n) {
    uint512_t m = n - 1;
    
}

uint512_t generate_safe_prime(PRNG& prng, size_t bit_length) {
    assert(bit_length <= 512);
    while(true) {
        
    }
}

int main() {
    PRNG prng;
    uint512_t num = generate_odd_uint512(prng, 512);
    std::cout << "Random 512-bit odd number: " << num << std::endl;
}