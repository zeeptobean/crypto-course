#ifndef CRYPTOCOURSE_PRNG_326_HPP
#define CRYPTOCOURSE_PRNG_326_HPP

#include <random>
#include <chrono>

class PRNG {
    uint64_t seed;
    std::mt19937_64 rng;
public:
    PRNG(const PRNG&) = delete;
    PRNG& operator=(const PRNG&) = delete;
    PRNG() {
        init();
    }
    PRNG(uint64_t seed) {
        init(seed);
    }
    void init(uint64_t seed) {
        this->seed = seed;
        rng.seed(seed);
    }
    void init() {
        seed = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        rng.seed(seed);
    }
    uint64_t next_u64() {
        return static_cast<uint64_t>(rng());
    }
    uint32_t next_u32() {
        return static_cast<uint32_t>(rng());
    }
    uint16_t next_u16() {
        return static_cast<uint16_t>(rng());
    }
};

#endif