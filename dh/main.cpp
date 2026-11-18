#include <iostream>
#include <cassert>
#include "prng.hpp"
#include "uint512.hpp"

uint512_t modular_exponentiation(uint512_t base, uint512_t exponent, const uint512_t& mod) {
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

uint512_t generate_uint512(PRNG& prng, size_t bit_length = 512) {
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
    return result;
}

uint512_t generate_odd_uint512(PRNG& prng, size_t bit_length = 512) {
    uint512_t result = generate_uint512(prng, bit_length);
    result.v[0] |= 1;                               // odd
    return result;
}

uint512_t generate_private_key(PRNG& prng, const uint512_t& p) {
    assert(p.v[0] > 5 && "p must be at least 5");
    uint512_t a = generate_uint512(prng);
    a = mod_uint512(a, p - 4) + 2; // random in [2, p-2]
    return a;
}

bool small_prime_test(const uint512_t& n) {
    const uint16_t small_primes[100] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
        73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
        179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
        283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
        419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541
    };
    for(size_t i = 0; i < 100; i++) {
        if(n == small_primes[i]) return true;
        uint64_t rem;
        div64_uint512(n, small_primes[i], rem);
        if(rem == 0) return false;
    }
    return true;
}

bool rabin_miller_test(PRNG& prng, const uint512_t& n, size_t k = 20) {
    uint512_t m = n - 1;
    uint512_t d = m;
    size_t s = 0;
    while(getbit_uint512(d, 0) == 0) {
        d = shift_right_uint512(d, 1);
        s++;
    }
    for(size_t i = 0; i < k; i++) {
        uint512_t a = generate_private_key(prng, n);
        // case 1: little fermat
        uint512_t x = modular_exponentiation(a, d, n);
        if(x == 1 || x == m) continue;

        // case 2: test
        bool composite = true;
        for(size_t j = 1; j < s; j++) {
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
    assert(8 <= bit_length <= 512);
    while(true) {
        uint512_t p = generate_odd_uint512(prng, bit_length-1);
        uint512_t q = (p << 1) + 1;
        if(!small_prime_test(q) || !small_prime_test(p)) {
            continue;
        }
        if(!rabin_miller_test(prng, q)) {
            continue;
        }
        std::cout << "Checked q: " << q << std::endl; 
        std::cout << "Checking p: " << p << std::endl; 
        if(!rabin_miller_test(prng, p)) {
            continue;
        }
        return q;
    }
}

int main() {
    PRNG prng(777);

    size_t bit_length = 512;

    // Generate a safe prime number might take too long, using a pre-generated number
    uint512_t p = generate_safe_prime(prng, bit_length);
    // uint512_t p = from_string_uint512("0xbe94ec64421e7b536b7a8c7903fe1f30e0c3898579df27b6a0cb52d6f036bc1342433ebcc12a1af11655c39827494b838a29ca8d33e6e1544203f7f58fb34497");
    uint512_t g = 2;

    uint512_t a = generate_private_key(prng, p);
    uint512_t b = generate_private_key(prng, p);

    uint512_t A = modular_exponentiation(g, a, p);
    uint512_t B = modular_exponentiation(g, b, p);

    uint512_t shared_secret_a = modular_exponentiation(B, a, p);
    uint512_t shared_secret_b = modular_exponentiation(A, b, p);
    
    std::cout << "Shared secret A: " << shared_secret_a << std::endl;
    std::cout << "Shared secret B: " << shared_secret_b << std::endl;
    std::cout << "Is equal? " << (shared_secret_a == shared_secret_b ? "Yes" : "No") << std::endl;

    return 0;
}