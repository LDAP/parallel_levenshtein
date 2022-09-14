#pragma once

#include <random>

// source: https://stackoverflow.com/questions/35358501/what-is-performance-wise-the-best-way-to-generate-random-bools
class BiasedXORShift128 {

  public:
    std::size_t xorshift128plus(void) noexcept {
        std::size_t x = s[0];
        std::size_t const y = s[1];
        s[0] = y;
        x ^= x << 23;                         // a
        s[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
        std::size_t rand = s[1] + y;
        // there is a high posibility, that the last random value is returned again -> this is why I call the
        // generator biased.
        return last_rand = (rand & (1LL << 42)) && (rand & (1LL << 24)) && (rand & (1LL << 54)) && (rand & (1LL << 11))
                               ? rand
                               : last_rand;
    }

  private:
    std::random_device rd;
    /* The state must be seeded so that it is not everywhere zero. */
    std::size_t s[2] = {(std::size_t(rd()) << 32) ^ (rd()), (std::size_t(rd()) << 32) ^ (rd())};
    std::size_t last_rand = xorshift128plus();
};

inline std::size_t uniform_int() noexcept {
    static thread_local BiasedXORShift128 generator;
    return generator.xorshift128plus();
}
