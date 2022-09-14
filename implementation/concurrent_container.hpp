#pragma once

#include "implementation/concurrent_queue.hpp"
#include "implementation/random.hpp"

#include <cstddef>
#include <memory>
#include <random>
#include <thread>
#include <vector>

template <class T> class alignas(CACHE_LINE_SIZE) ConcurrentContainer {
  public:
    ConcurrentContainer(std::size_t capacity, std::size_t num_threads = std::thread::hardware_concurrency())
        : num_queues(next_power_of_two(num_threads * 2)), max_queue_index(num_queues - 1) {

        std::size_t elements_per_queue = (capacity + num_queues) / num_queues;

        for (int t = 0; t < num_queues; t++) {
            queues.emplace_back(elements_per_queue);
        }
    }

    void push(const T& e) noexcept {
        int index = uniform_int() & max_queue_index;
        while (!queues[index].try_push(e)) {
            index = (index + 1) & max_queue_index;
        }
    }

    T pop() noexcept {
        T e = ConcurrentQueue<T>::EMPTY;
        int index = uniform_int() & max_queue_index;
        while (!queues[index].try_pop(e)) {
            index = (index + 1) & max_queue_index;
        }
        return e;
    }

    // assert e == EMPTY
    bool try_pop(T& e) noexcept {
        int index = uniform_int() & max_queue_index;
        int stop = index;
        while (!queues[index].try_pop(e)) {
            index = (index + 1) & max_queue_index;
            if (index == stop)
                return false;
        }
        return true;
    }

  private:
    std::vector<ConcurrentQueue<T>> queues;
    const int num_queues;
    const int max_queue_index;
};
