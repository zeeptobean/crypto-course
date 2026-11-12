#include <iostream>
#include <cassert>
#include "prng.hpp"
#include "uint512.hpp"

struct Mont {
    uint512_t m, one;   // one = R mod m  (Montgomery form of 1)
    uint512_t r2;       // R^2 mod m, R = 2^512
    uint64_t  ninv;     // -m^{-1} mod 2^64
};

static inline uint512_t sub_wrap(const uint512_t& a, const uint512_t& b) {
    uint512_t r; uint64_t br = 0;
    for (int i = 0; i < 8; i++) {
        uint64_t d = a.v[i] - b.v[i], b1 = a.v[i] < b.v[i];
        uint64_t d2 = d - br,         b2 = d < br;
        r.v[i] = d2; br = b1 | b2;
    }
    return r;
}

static inline Mont mont_init(const uint512_t& m) {
    assert((m.v[0] & 1) && "Montgomery needs odd modulus");
    Mont M; M.m = m;
    uint64_t x = m.v[0];                       // Newton: 3 -> 6 -> 12 -> 24 -> 48 -> 96 bits
    for (int i = 0; i < 5; i++) x *= 2 - m.v[0] * x;
    M.ninv = -x;

    uint512_t zero;
    M.one = mod_uint512(sub_wrap(zero, m), m); // 2^512 - m  ≡  R (mod m)
    uint512_t t = M.one;                       // double 512 times -> R * 2^512 = R^2
    for (int i = 0; i < 512; i++) {
        uint64_t top = t.v[7] >> 63;
        t = shift_left_uint512(t, 1);
        if (top || compare_uint512(t, m) >= 0) t = sub_wrap(t, m);
    }
    M.r2 = t;
    return M;
}

static inline uint512_t mont_mul(const uint512_t& a, const uint512_t& b, const Mont& M) {
    uint64_t t[10] = {0};
    for (int i = 0; i < 8; i++) {
        uint64_t c = 0;
        for (int j = 0; j < 8; j++) {
            uint128_t p = (uint128_t)a.v[j] * b.v[i] + t[j] + c;
            t[j] = (uint64_t)p; c = (uint64_t)(p >> 64);
        }
        uint128_t s = (uint128_t)t[8] + c;
        t[8] = (uint64_t)s; t[9] = (uint64_t)(s >> 64);

        uint64_t q = t[0] * M.ninv;
        uint128_t p = (uint128_t)q * M.m.v[0] + t[0];
        c = (uint64_t)(p >> 64);
        for (int j = 1; j < 8; j++) {
            p = (uint128_t)q * M.m.v[j] + t[j] + c;
            t[j-1] = (uint64_t)p; c = (uint64_t)(p >> 64);
        }
        s = (uint128_t)t[8] + c;
        t[7] = (uint64_t)s; t[8] = t[9] + (uint64_t)(s >> 64);
    }
    uint512_t r;
    for (int i = 0; i < 8; i++) r.v[i] = t[i];
    if (t[8] || compare_uint512(r, M.m) >= 0) r = sub_wrap(r, M.m);
    return r;
}

uint512_t modular_exponentiation(uint512_t base, const uint512_t& exp, const uint512_t& mod) {
    Mont M = mont_init(mod);                   // cache this if mod is reused
    base = mod_uint512(base, mod);

    uint512_t tbl[16];                         // tbl[k] = base^k in Montgomery form
    tbl[0] = M.one;
    tbl[1] = mont_mul(base, M.r2, M);
    for (int k = 2; k < 16; k++) tbl[k] = mont_mul(tbl[k-1], tbl[1], M);

    uint512_t acc = M.one;
    for (int w = 127; w >= 0; w--) {           // 128 nibbles, top first
        for (int s = 0; s < 4; s++) acc = mont_mul(acc, acc, M);
        unsigned nib = (exp.v[w / 16] >> ((w % 16) * 4)) & 0xF;
        if (nib) acc = mont_mul(acc, tbl[nib], M);
    }
    return mont_mul(acc, uint512_t(1), M);     // leave Montgomery form
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
        uint512_t a = generate512_uint512(prng);
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
        if(!rabin_miller_test(prng, q)) {
            continue;
        }
        uint512_t p = (q - 1) >> 1;
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
    uint512_t num = generate_safe_prime(prng, 512);
    std::cout << "A prime number: " << num << std::endl;
    std::cout << "In decimal: " << to_string_decimal_uint512(num) << std::endl;
    return 0;
}