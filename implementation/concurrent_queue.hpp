#pragma once

#include "implementation/locking.hpp"

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

inline std::size_t next_power_of_two(std::size_t number) {
    for (std::size_t power = 1;; power *= 2) {
        if (power >= number)
            return power;
    }
    return -1;
}

template <class T> class alignas(CACHE_LINE_SIZE) ConcurrentQueue {
  public:
    static constexpr T EMPTY = T();

    ConcurrentQueue(std::size_t capacity) : size(next_power_of_two(capacity)), head(0), tail(0) {
        v = new std::atomic<T>[size];
        for (std::size_t i = 0; i < size; i++)
            v[i].store(EMPTY, std::memory_order_relaxed);
    }

    ConcurrentQueue() = delete;
    ConcurrentQueue(const ConcurrentQueue& q) {
        size = q.size;
        head.store(q.head, std::memory_order_relaxed);
        tail.store(q.tail, std::memory_order_relaxed);
        v = new std::atomic<T>[q.size];
        for (std::size_t i = 0; i < q.size; i++)
            v[i].store(q.v[i], std::memory_order_relaxed);
    }

    ~ConcurrentQueue() {
        delete[] v;
    }

    ConcurrentQueue& operator=(const ConcurrentQueue& src) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue&& src) {
        // magic move assignment operator
        // don't think about it too much
        if (this == &src)
            return *this;
        this->~ConcurrentQueue();
        new (this) ConcurrentQueue(std::move(src));
        return *this;
    }

    void push(const T& e) noexcept {
        while (!try_push(e)) {
            _pause();
        }
    }

    T pop() noexcept {
        T element = EMPTY;
        while (!try_pop(element))
            _pause();
        return element;
    }

    // fails if queue is full -> allows changing to wait in push/pop
    inline bool try_push(const T& e) noexcept {
        std::size_t h = LOAD_REL(head);

        // Update head and claim index
        do {
            if (h >= LOAD_REL(tail) + size) // full
                return false;
        } while (!CES_ADD_ONE_AQ(head, h));

        std::atomic<T>& push_entry = v[index(h)];

        // Actual push
        T empty = EMPTY;
        while (!push_entry.compare_exchange_strong(empty, e, std::memory_order_release, std::memory_order_relaxed)) {

            // wait until entry is free
            PAUSE_WHILE(LOAD_REL(push_entry) != EMPTY)

            // compare_exchange_strong overwrites this value
            empty = EMPTY;
        }

        return true;
    }

    // fails if queue is empty -> allows changing to wait in push/pop
    // assert e == EMPTY
    inline bool try_pop(T& e) noexcept {
        std::size_t t = LOAD_REL(tail);

        // Update tail and claim index
        do {
            if (LOAD_REL(head) <= t)
                return false;
        } while (!CES_ADD_ONE_AQ(tail, t));

        std::atomic<T>& pop_entry = v[index(t)];

        // Actual pop
        while (true) {
            e = pop_entry.exchange(EMPTY, std::memory_order_release);
            if (e != EMPTY)
                return true;

            // element is available
            PAUSE_WHILE(LOAD_REL(pop_entry) == EMPTY)
        }
        return true;
    }

  private:
    alignas(CACHE_LINE_SIZE) std::size_t size;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> head;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> tail;
    alignas(CACHE_LINE_SIZE) std::atomic<T>* v;

    inline std::size_t index(std::size_t i) noexcept {
        // maybe add randomness to prevent accesses to same cache line
        return i & (size - 1);
    }
};
