#ifndef CRYPTOCOURSE_UINT512_326_HPP
#define CRYPTOCOURSE_UINT512_326_HPP

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <string>

struct uint512_t {
    uint64_t v[8]{0};

    uint512_t() = default;

    uint512_t(uint64_t value) {
        // memset(v, 0, sizeof(v));
        v[0] = value;
    }

    uint512_t operator=(uint64_t value) {
        memset(v, 0, sizeof(v));
        v[0] = value;
        return *this;
    }
};

int compare_uint512(const uint512_t& a, const uint512_t& b) {
    for(int i = 7; i >= 0; i--) {
        if (a.v[i] < b.v[i]) return -1;
        if (a.v[i] > b.v[i]) return 1;
    }
    return 0;
}
uint512_t add_uint512(const uint512_t& a, const uint512_t& b) {
    uint512_t result;
    uint64_t carry = 0;
    
    for(int i = 0; i < 8; i++) {
        uint64_t sum1 = a.v[i] + carry;
        uint64_t carry1 = (sum1 < a.v[i]) ? 1 : 0;
        
        uint64_t sum2 = sum1 + b.v[i];
        uint64_t carry2 = (sum2 < sum1) ? 1 : 0;
        
        result.v[i] = sum2;
        carry = carry1 + carry2;
    }
    return result;
}
// Underflow is NOT permitted, hence a check
uint512_t sub_uint512(const uint512_t& a, const uint512_t& b) {
    assert(compare_uint512(a, b) >= 0 && "a must be greater than or equal to b for subtraction");
    uint512_t result;
    for(int i = 0; i < 8; ++i) {
        result.v[i] = a.v[i] - b.v[i];
    }
    return result;
}

uint512_t mul64_uint512(const uint512_t& a, uint64_t b) {
    uint512_t result;
    uint64_t carry = 0;

    for(int i = 0; i < 8; i++) {
        __uint128_t product = (__uint128_t)a.v[i] * b + carry;
        result.v[i] = (uint64_t)product;
        carry = (uint64_t)(product >> 64);
    }
    return result;
}

uint512_t div64_uint512(const uint512_t& a, uint64_t b, uint64_t& rem) {
    uint512_t quotient;
    __uint128_t current = 0;

    for (int i = 7; i >= 0; --i) {
        current = (current << 64) | a.v[i];
        quotient.v[i] = static_cast<uint64_t>(current / b);
        current %= b;
    }

    rem = static_cast<uint64_t>(current);
    return quotient;
}

bool getbit_uint512(const uint512_t& a, size_t index) {
    if(index >= 512) return 0;
    size_t word_index = index / 64;
    size_t bit_index = index % 64;
    return (a.v[word_index] >> bit_index) & 1;
}

uint512_t getbit512_uint512(const uint512_t& a, size_t index) {
    return (uint64_t) getbit_uint512(a, index);
}

void setbit_uint512(uint512_t& a, size_t index, bool value = 1) {
    assert(index < 512 && "setbit: index out of bound");
    size_t word_index = index / 64;
    size_t bit_index = index % 64;
    if(value) {
        a.v[word_index] |= (1ULL << bit_index);
    } else {
        a.v[word_index] &= ~(1ULL << bit_index);
    }
}

uint512_t shift_left_uint512(const uint512_t& a, size_t shift) {
    uint512_t result;
    if(shift >= 512) return result;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;

    for(int i = 7; i >= 0; i--) {
        int src_index = i - word_shift;
        if(src_index >= 0) {
            result.v[i] = a.v[src_index] << bit_shift;
            if (bit_shift > 0 && src_index - 1 >= 0) {
                result.v[i] |= a.v[src_index - 1] >> (64 - bit_shift);
            }
        } else {
            result.v[i] = 0;
        }
    }
    return result;
}
uint512_t shift_right_uint512(const uint512_t& a, size_t shift) {
    uint512_t result;
    if(shift >= 512) return result;
    int word_shift = shift / 64;
    int bit_shift = shift % 64;

    for(int i = 0; i < 8; i++) {
        result.v[i] = a.v[i + word_shift] >> bit_shift;
        if (bit_shift > 0 && i + word_shift + 1 < 8) {
            result.v[i] |= a.v[i + word_shift + 1] << (64 - bit_shift);
        }
    }
    return result;
}

// Single bit shifting modular, work in all case
uint512_t mod_uint512(const uint512_t& a, const uint512_t& m) {
    uint512_t rem;
    for(int i=511; i >= 0; i--) {
        rem = shift_left_uint512(rem, 1);
        uint512_t xi = getbit512_uint512(a, i); 
        rem = add_uint512(rem, xi);
        if(compare_uint512(rem, m) >= 0) {
            rem = sub_uint512(rem, m);
        }
    }
    return rem;
}

// m must be 511 bit (2^511 <= m < 2^512), so modular is at most one subtraction
uint512_t mod512_uint512(const uint512_t& a, const uint512_t& m) {
    assert(m.v[7] >> 63 == 1 && "m must be 511 bit (2^511 <= m < 2^512)");
    if(compare_uint512(a, m) >= 0) {
        return sub_uint512(a, m);
    } else {
        return a;
    }
}

uint512_t add_mod_uint512(uint512_t a, uint512_t b, const uint512_t& mod) {
    a = mod_uint512(a, mod);
    b = mod_uint512(b, mod);
    if(compare_uint512(a, sub_uint512(mod, b)) >= 0) {
        return sub_uint512(a, sub_uint512(mod, b));
    } else {
        return add_uint512(a, b);
    }
}
uint512_t mul_mod_uint512(uint512_t a, const uint512_t& b, const uint512_t& mod) {
    uint512_t result;
    a = mod_uint512(a, mod);
    for(int i=0; i < 512; i++) {
        if(getbit_uint512(b, i)) {
            result = add_mod_uint512(result, a, mod);
        }
        a = add_mod_uint512(a, a, mod);
    }
    return result;
}

void print_uint512(const uint512_t& a) {
    for(int i = 7; i >= 0; i--) {
        printf("%016llx", a.v[i]);
    }
    puts("");
}

std::string to_string_uint512(const uint512_t& a) {
    char buffer[135];
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "0x");
    for(int i = 7, c = 0; i >= 0; i--, c+=16) {
        sprintf(buffer+c, "%016llx", a.v[i]);
    }
    return std::string(buffer);
}

std::string to_string_decimal_uint512(uint512_t val) {
    if (compare_uint512(val, uint512_t(0)) == 0) {
        return "0";
    }

    std::string result;
    while (compare_uint512(val, uint512_t(0)) != 0) {
        uint64_t remainder;
        val = div64_uint512(val, 10, remainder);
        result += char('0' + remainder);
    }

    std::string result2;
    result.reserve(result.size());
    for (auto it = result.rbegin(); it != result.rend(); ++it) {
        result2 += *it;
    }
    return result2;
}

// Parses a uint512_t from a hexadecimal or decimal string (base 16 or base 10)
uint512_t from_string_uint512(const std::string& s) {
    uint512_t result;
    size_t start = 0;
    int base = 10;

    // Check for "0x" or "0X" prefix for hex
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        start = 2;
    } else if (s.length() == 1 && s[0] == '0') {
        return result;
    } else {
        // Try to see if it contains hex characters or if it's purely decimal. 
        // Usually, if it starts with 0x it's hex, otherwise support standard hex or dec.
        // Let's support base 16 if it has any a-f/A-F or if requested, but standard is checking prefix or parsing char by char.
        // A robust way: check if all characters are valid hex or decimal. If we want base 2 or base 16/10:
        // The user asked "write from string function for this base 2 512bit bigint" -> wait, "base 2 512bit bigint" might mean base 16 (hex) string representation like `to_string_uint512`, or literal binary string ("0110..."), or decimal/hex. Let's support both hex, decimal, and binary strings just in case!
    }

    // Let's inspect if the string is binary ('0'/'1'), hex, or decimal.
    // If it contains only '0' and '1' (and optional 0b prefix), treat as binary.
    bool is_binary = true;
    size_t bin_start = 0;
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        bin_start = 2;
    }
    if (s.length() == bin_start) is_binary = false;
    for (size_t i = bin_start; i < s.length(); ++i) {
        if (s[i] != '0' && s[i] != '1') {
            is_binary = false;
            break;
        }
    }

    if (is_binary && s.length() > bin_start) {
        for (size_t i = bin_start; i < s.length(); ++i) {
            result = shift_left_uint512(result, 1);
            if (s[i] == '1') {
                result.v[0] |= 1ULL;
            }
        }
        return result;
    }

    // Check hex
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        start = 2;
        for (size_t i = start; i < s.length(); ++i) {
            char c = s[i];
            uint8_t val = 0;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
            else break;
            
            result = shift_left_uint512(result, 4);
            result.v[0] |= val;
        }
        return result;
    }

    // If it has a-f characters anywhere, treat as hex without prefix
    bool has_hex_chars = false;
    for (char c : s) {
        if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            has_hex_chars = true;
            break;
        }
    }

    if (has_hex_chars) {
        for (char c : s) {
            uint8_t val = 0;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
            else continue;

            result = shift_left_uint512(result, 4);
            result.v[0] |= val;
        }
        return result;
    }

    // Otherwise, treat as decimal string
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            uint512_t digit = uint64_t(c - '0');
            // result = result * 10 + digit
            // Since we don't have direct multiplication by 10, we can do result * 8 + result * 2 + digit
            uint512_t r8 = shift_left_uint512(result, 3);
            uint512_t r2 = shift_left_uint512(result, 1);
            result = add_uint512(add_uint512(r8, r2), digit);
        }
    }

    return result;
}

std::ostream& operator<<(std::ostream& os, const uint512_t& a) {
    os << to_string_uint512(a);
    return os;
}

uint512_t operator+(const uint512_t& a, const uint512_t& b) {
    return add_uint512(a, b);
}

uint512_t operator-(const uint512_t& a, const uint512_t& b) {
    return sub_uint512(a, b);
}

uint512_t operator*(const uint512_t& a, uint64_t b) {
    return mul64_uint512(a, b);
}

uint512_t operator%(const uint512_t& a, const uint512_t& b) {
    return mod_uint512(a, b);
}

uint512_t operator/(const uint512_t& a, uint64_t b) {
    uint64_t rem;
    return div64_uint512(a, b, rem);
}

uint64_t operator%(const uint512_t& a, uint64_t b) {
    uint64_t rem;
    div64_uint512(a, b, rem);
    return rem;
}

uint512_t operator<<(const uint512_t& a, size_t shift) {
    return shift_left_uint512(a, shift);
}

uint512_t operator>>(const uint512_t& a, size_t shift) {
    return shift_right_uint512(a, shift);
}

bool operator<(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) < 0;
}

bool operator>(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) > 0;
}

bool operator<=(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) <= 0;
}

bool operator>=(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) >= 0;
}

bool operator==(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) == 0;
}

bool operator!=(const uint512_t& a, const uint512_t& b) {
    return compare_uint512(a, b) != 0;
}

#endif