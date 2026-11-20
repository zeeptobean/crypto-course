#ifndef CRYPTOCOURSE_UBIGINT2_HPP
#define CRYPTOCOURSE_UBIGINT2_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <exception>

template <size_t L>
class UBigInt2 {
    static_assert(L > 1, "look like 1 is not fine");

    uint64_t v[L]{0};
    
    public:

    UBigInt2<L>() = default;
    UBigInt2<L>(uint64_t value) {
        v[0] = value;
    }
    // explicit UBigInt2<L>(unsigned __int128 value) {
    //     v[0] = static_cast<uint64_t>(value);
    //     v[1] = static_cast<uint64_t>(value >> 64);
    // }
    UBigInt2<L>(const std::string& s) {
        from_string(s);
    }

    UBigInt2<L>(const UBigInt2<L>& other) {
        memcpy(v, other.v, sizeof(v));
    }

    UBigInt2<L>& operator=(const UBigInt2<L>& other) {
        if (this != &other) {
            memcpy(v, other.v, sizeof(v));
        }
        return *this;
    }

    UBigInt2<L>& operator=(const uint64_t& other) {
        memset(v, 0, sizeof(v));
        v[0] = other;
        return *this;
    }

    UBigInt2<L>& operator=(UBigInt2<L>&& other) {
        if (this != &other) {
            memcpy(v, other.v, sizeof(v));
        }
        return *this;
    }

    void from_string(const std::string& s) {
        UBigInt2<L> result;
        int base = 2;
        size_t start = 0;

        if(s.length() == 1 && s[0] == '0') {
            return;
        } else if(s.length() > 2 && (s[0] == '0' && (s[1] == 'b' || s[1] == 'B'))) {
            //explicit binary
            base = 2;
            start = 2;
            for(size_t i = 2; i < s.length(); ++i) {
                char c = s[i];
                if(c != '0' && c != '1') {
                    throw std::invalid_argument("UBigInt2::from_string(): Invalid character in input string");
                }
            }
        } else if(s.length() > 2 && (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {
            //explicit hex
            base = 16;
            start = 2;
            for(size_t i = 2; i < s.length(); ++i) {
                char c = s[i];
                if(!isxdigit(c)) {
                    throw std::invalid_argument("UBigInt2::from_string(): Invalid character in input string");
                }
            }
        } else {
            //auto-detect hex or decimal
            for(const char& c:s) {
                unsigned char uc = static_cast<unsigned char>(c);
                if (!std::isxdigit(uc)) {
                    throw std::invalid_argument("UBigInt2::from_string(): Invalid character in input string");
                }
                if (!std::isdigit(uc)) {
                    base = 16;
                }
            }
        }

        if (start == s.size()) {
            throw std::invalid_argument("UBigInt2::from_string(): Missing digits after base prefix");
        }
        // Skip leading zeros
        while (start < s.size() && s[start] == '0') {
            ++start;
        }
        if (start == s.size()) {
            return;
        }

        if(base == 2) {
            if(s.size() - start > 512) {
                throw std::invalid_argument("UBigInt2::from_string(): Binary number too large");
            }
            for(size_t i = start; i < s.length(); i++) {
                result = shift_left_uint512(result, 1);
                if (s[i] == '1') {
                    result.v[0] |= 1ULL;
                }
            }
        } else if(base == 10) {
            for(size_t i = start; i < s.length(); i++) {
                UBigInt2<L> digit = uint64_t(s[i] - '0');
                // a = a * 10 + d = (a << 3) + (a << 1) + d
                UBigInt2<L> r8 = result << 3;
                UBigInt2<L> r2 = result << 1;
                result = r8 + r2 + digit;
            }
        } else if(base == 16) {
            if(s.size() - start > 128) {
                throw std::invalid_argument("UBigInt2::from_string(): Hexadecimal number too large");
            }
            for(size_t i = start; i < s.length(); i++) {
                char c = s[i];
                uint8_t val = 0;
                if (c >= '0' && c <= '9') val = c - '0';
                else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
                else throw std::invalid_argument("UBigInt2::from_string(): Invalid character in input string");
                
                result = shift_left_uint512(result, 4);
                result.v[0] |= val;
            }
        } 
        *this = result;
    }

    std::string to_string() const {
        if(compare(0) == 0) {
            return "0";
        }

        UBigInt2<L> tmp = *this;
        std::string result;
        while(tmp != 0) {
            uint64_t rem;
            tmp.div64(10, rem);
            result += char('0' + rem);
        }

        std::string result2;
        result.reserve(result.size());
        for (auto it = result.rbegin(); it != result.rend(); ++it) {
            result2 += *it;
        }
        return result2;
    }

    std::string to_hex_string() const {
        char buf[20];
        memset(buf, 0, sizeof(buf));
        std::string output;
        output.reserve(L*16 + 2);
        output += "0x";
        for(int i=L-1; i >= 0; i--) {
            sprintf(buf, "%016llx", (unsigned long long) v[i]);
            output += buf;
        }
        return output;
    }

    void set_limb(size_t index, uint64_t value) {
        if(index >= L) throw std::invalid_argument("UBigInt2::set_limb: index out of bound");
        v[index] = value;
    }

    uint64_t& get_limb(size_t index) {
        if(index >= L) throw std::invalid_argument("UBigInt2::get_limb: index out of bound");
        return v[index];
    }

    int compare(const UBigInt2<L>& rhs) const {
        for(int i = L - 1; i >= 0; i--) {
            if (v[i] < rhs.v[i]) return -1;
            if (v[i] > rhs.v[i]) return 1;
        }
        return 0;
    }

    UBigInt2<L>& bitwise_xor(const UBigInt2<L>& rhs) {
        for(size_t i = 0; i < L; i++) {
            v[i] ^= rhs.v[i];
        }
        return *this;
    }

    UBigInt2<L>& bitwise_and(const UBigInt2<L>& rhs) {
        for(size_t i = 0; i < L; i++) {
            v[i] &= rhs.v[i];
        }
        return *this;
    }

    UBigInt2<L>& bitwise_or(const UBigInt2<L>& rhs) {
        for(size_t i = 0; i < L; i++) {
            v[i] |= rhs.v[i];
        }
        return *this;
    }

    UBigInt2<L>& bitwise_not() {
        for(size_t i = 0; i < L; i++) {
            v[i] = ~v[i];
        }
        return *this;
    }

    UBigInt2<L>& add(const UBigInt2<L>& rhs) {
        uint64_t carry = 0;
        
        for(size_t i = 0; i < L; i++) {
            uint64_t sum1 = v[i] + carry;
            uint64_t carry1 = (sum1 < v[i]) ? 1 : 0;
            
            uint64_t sum2 = sum1 + rhs.v[i];
            uint64_t carry2 = (sum2 < sum1) ? 1 : 0;
            
            v[i] = sum2;
            carry = carry1 + carry2;
        }
        return *this;
    }

    UBigInt2<L>& sub(const UBigInt2<L>& rhs) {
        if(compare(rhs) < 0) {
            throw std::invalid_argument("UBigInt2::sub: Underflow is not permitted, a must be greater than or equal to b");
        }
        uint64_t borrow = 0;
        for (size_t i = 0; i < L; i++) {
            uint64_t d  = v[i] - rhs.v[i];
            uint64_t b1 = (v[i] < rhs.v[i]) ? 1 : 0;
            uint64_t d2 = d - borrow;
            uint64_t b2 = (d < borrow) ? 1 : 0;
            v[i] = d2;
            borrow = b1 | b2;
        }
        return *this;
    }

    UBigInt2<L>& shift_left(size_t shift) {
        if(shift >= L*64) {
            *this = 0;
            return *this;
        }
        size_t word_shift = shift / 64;
        size_t bit_shift = shift % 64;

        UBigInt2<L> a = *this;
        for(int i = L-1; i >= 0; i--) {
            int src_index = i - word_shift;
            if(src_index >= 0) {
                v[i] = a.v[src_index] << bit_shift;
                if (bit_shift > 0 && src_index - 1 >= 0) {
                    v[i] |= a.v[src_index - 1] >> (64 - bit_shift);
                }
            } else {
                v[i] = 0;
            }
        }
        return *this;
    }

    UBigInt2<L>& shift_right(size_t shift) {
        if(shift >= L*64) {
            *this = 0;
            return *this;
        }
        size_t word_shift = shift / 64;
        size_t bit_shift = shift % 64;
        UBigInt2<L> a = *this;

        for(size_t i = 0; i < L; i++) {
            v[i] = a.v[i + word_shift] >> bit_shift;
            if (bit_shift > 0 && i + word_shift + 1 < L) {
                v[i] |= a.v[i + word_shift + 1] << (64 - bit_shift);
            }
        }
        return *this;
    }

    bool get_bit(size_t index) const {
        if(index >= L * 64) return false;
        size_t word_index = index / 64;
        size_t bit_index = index % 64;
        return (v[word_index] >> bit_index) & 1;
    }

    void set_bit(size_t index, bool value = true) {
        if(index >= L * 64) throw std::invalid_argument("UBigInt2::set_bit: index out of bound");
        size_t word_index = index / 64;
        size_t bit_index = index % 64;
        if(value) {
            v[word_index] |= (1ULL << bit_index);
        } else {
            v[word_index] &= ~(1ULL << bit_index);
        }
    }

    UBigInt2<L>& mul64(uint64_t rhs) {
        uint64_t carry = 0;
        for(size_t i = 0; i < L; i++) {
            unsigned __int128 product = (unsigned __int128) v[i] * rhs + carry;
            v[i] = (uint64_t)product;
            carry = (uint64_t)(product >> 64);
        }
        return *this;
    }

    UBigInt2<L>& div64(const uint64_t& rhs, uint64_t& rem) {
        if(rhs == 0) {
            throw std::invalid_argument("UBigInt2::div64: division by zero");
        }
        unsigned __int128 current = 0;
        for(int i = L-1; i >= 0; i--) {
            current = (current << 64) | v[i];
            v[i] = (uint64_t)(current / rhs);
            current %= rhs;
        }
        rem = static_cast<uint64_t>(current);
        return *this;
    }

    UBigInt2<L>& mod(const UBigInt2<L>& rhs) {
        if(rhs == 0) {
            throw std::invalid_argument("UBigInt2::mod: modulus cannot be zero");
        }
        if(*this < rhs) {
            return *this;
        }
        UBigInt2<L> rem;
        for(int i=L*64-1; i >= 0; i--) {
            rem.shift_left(1);
            UBigInt2<L> xi = get_bit(i); 
            rem.add(xi);
            if(rem.compare(rhs) >= 0) {
                rem.sub(rhs);
            }
        }
        *this = rem;
        return *this;
    }

    UBigInt2<L>& add_mod(UBigInt2<L> rhs, const UBigInt2<L>& m) {
        mod(m);
        rhs.mod(m);
        UBigInt2<L> sub_b = m - rhs;
        if(compare(sub_b) >= 0) {
            *this = *this - sub_b;
        } else {
            *this = *this + rhs;
        }
        return *this;
    }

    UBigInt2<L>& mul_mod(const UBigInt2<L>& rhs, const UBigInt2<L>& m) {
        UBigInt2<L> result;
        mod(m);
        for(size_t i = 0; i < L * 64; i++) {
            if(rhs.get_bit(i)) {
                result.add_mod(*this, m);
            }
            add_mod(*this, m);
        }
        *this = result;
        return *this;
    }

    UBigInt2<L> operator+(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.add(rhs);
        return result;
    }

    UBigInt2<L> operator-(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.sub(rhs);
        return result;
    }

    UBigInt2<L> operator*(uint64_t rhs) const {
        UBigInt2<L> result = *this;
        result.mul64(rhs);
        return result;
    }

    UBigInt2<L> operator/(uint64_t rhs) const {
        UBigInt2<L> result = *this;
        uint64_t rem;
        result.div64(rhs, rem);
        return result;
    }

    uint64_t operator%(uint64_t rhs) const {
        UBigInt2<L> result = *this;
        uint64_t rem;
        (void) result.div64(rhs, rem);
        return rem;
    }

    UBigInt2<L> operator%(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.mod(rhs);
        return result;
    }

    UBigInt2<L> operator<<(size_t shift) const {
        UBigInt2<L> result = *this;
        result.shift_left(shift);
        return result;
    }

    UBigInt2<L> operator>>(size_t shift) const {
        UBigInt2<L> result = *this;
        result.shift_right(shift);
        return result;
    }

    UBigInt2<L> operator^(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.bitwise_xor(rhs);
        return result;
    }

    UBigInt2<L> operator&(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.bitwise_and(rhs);
        return result;
    }

    UBigInt2<L> operator|(const UBigInt2<L>& rhs) const {
        UBigInt2<L> result = *this;
        result.bitwise_or(rhs);
        return result;
    }

    bool operator<(const UBigInt2<L>& rhs) const {
        return compare(rhs) < 0;
    }

    bool operator>(const UBigInt2<L>& rhs) const {
        return compare(rhs) > 0;
    }

    bool operator<=(const UBigInt2<L>& rhs) const {
        return compare(rhs) <= 0;
    }

    bool operator>=(const UBigInt2<L>& rhs) const {
        return compare(rhs) >= 0;
    }

    bool operator==(const UBigInt2<L>& rhs) const {
        return compare(rhs) == 0;
    }

    bool operator!=(const UBigInt2<L>& rhs) const {
        return compare(rhs) != 0;
    }

    UBigInt2<L>& operator^=(const UBigInt2<L>& rhs) {
        return bitwise_xor(rhs);
    }

    UBigInt2<L>& operator&=(const UBigInt2<L>& rhs) {
        return bitwise_and(rhs);
    }

    UBigInt2<L>& operator|=(const UBigInt2<L>& rhs) {
        return bitwise_or(rhs);
    }

    UBigInt2<L> operator~() const {
        UBigInt2<L> result = *this;
        return result.bitwise_not();
    }

    UBigInt2<L>& operator+=(const UBigInt2<L>& rhs) {
        return add(rhs);
    }

    UBigInt2<L>& operator-=(const UBigInt2<L>& rhs) {
        return sub(rhs);
    }

    UBigInt2<L>& operator*=(uint64_t rhs) {
        return mul64(rhs);
    }

    UBigInt2<L>& operator/=(uint64_t rhs) {
        uint64_t rem;
        return div64(rhs, rem);
    }

    UBigInt2<L>& operator%=(const UBigInt2<L>& rhs) {
        return mod(rhs);
    }

    UBigInt2<L>& operator<<=(size_t shift) {
        return shift_left(shift);
    }

    UBigInt2<L>& operator>>=(size_t shift) {
        return shift_right(shift);
    }

    friend std::ostream& operator<<(std::ostream& os, const UBigInt2<L>& n) {
        // Isolate the basefield bits (dec, hex, oct)
        auto base = os.flags() & std::ios_base::basefield;

        if (base == std::ios_base::hex) {
            os << n.to_hex_string();
        } else if (base == std::ios_base::oct) {
            throw std::invalid_argument("UBigInt2: Octal output not supported");
        } else {
            os << n.to_string();
        }
        return os;
    }

    friend std::istream& operator>>(std::istream& is, UBigInt2<L>& n) {
        std::string s;
        is >> s;
        n.from_string(s);
        return is;
    }
};

using uint512_t = UBigInt2<8>;
using uint1024_t = UBigInt2<16>;

#endif