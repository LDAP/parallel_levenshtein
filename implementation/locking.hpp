#pragma once

#include <atomic>
#include <vector>

#define PAUSE_WHILE(expr)                                                                                              \
    do                                                                                                                 \
        _pause();                                                                                                      \
    while (expr);
#define CES_ADD_ONE_AQ(atomic, var)                                                                                    \
    atomic.compare_exchange_strong(var, var + 1, std::memory_order_acquire, std::memory_order_relaxed)
#define LOAD_REL(atomic) atomic.load(std::memory_order_relaxed)

constexpr int CACHE_LINE_SIZE = 128;

inline void _pause() noexcept {
    __builtin_ia32_pause();
}

inline void lock_atomic(std::atomic<bool>& lock) {
    for (;;) {
        if (!lock.exchange(true, std::memory_order_acquire)) {
            return;
        }
        while (lock.load(std::memory_order_relaxed)) {
            // X86 PAUSE, ARM YIELD instruction
            __builtin_ia32_pause();
        }
    }
}

inline void unlock_atomic(std::atomic<bool>& lock) {
    lock.store(false, std::memory_order_release);
}

template <class VectorType> inline void push(std::vector<VectorType>& v, VectorType& element, std::atomic<bool>& lock) {
    lock_atomic(lock);
    v.push_back(element);
    unlock_atomic(lock);
}