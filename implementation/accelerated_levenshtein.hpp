#pragma once

#include "implementation/concurrent_queue.hpp"
#include "implementation/levenshtein_penalty_functions.hpp"
#include "implementation/locking.hpp"
#include "implementation/trie.hpp"
#include "implementation/vectorized_trie.hpp"
#include "utils/statistics_collector.hpp"

#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct TriePayload {
    std::size_t num_children;
    float min_distance;
    float distance;
};

// set early break to true to skip sub-trees that cannot be better that the
// current best
template <class PenaltyClass = levenshtein::KBDistance, class ParallelTrieImpl = VectorizedParallelTrie<TriePayload>,
          bool early_break = true, bool collect_stats = false>
class AcceleratedLevenshtein {

  private:
    using NodePtrType = typename ParallelTrieImpl::NodePtrType;
    using ChildPtrIteratorType = typename ParallelTrieImpl::ChildPtrIteratorType;

  public:
    AcceleratedLevenshtein(PenaltyClass penalty, std::size_t num_threads = std::thread::hardware_concurrency())
        : penalty(penalty), num_threads(num_threads), trie(num_threads) {}

    std::size_t compute_number_children(NodePtrType ptr) {
        ChildPtrIteratorType it;
        ChildPtrIteratorType end;

        std::tie(it, end) = trie.get_child_iterator(ptr);

        if (it == end)
            return trie.get_payload(ptr).num_children = 0;

        std::size_t count = 0;

        for (; it != end; it++) {
            count += compute_number_children(trie.dereference_child_iterator(it)) + 1;
        }

        trie.get_payload(ptr).num_children = count;

        return count;
    }

    void precompute(std::vector<std::string>& words) noexcept {
        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("insert");
        }
        trie.insert(words);
        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }

        if constexpr (early_break) {
            // count children for early stop

            if constexpr (collect_stats) {
                statistics_collector::get().start_measure("compute_children");
            }
            compute_number_children(trie.get_root());
            if constexpr (collect_stats) {
                statistics_collector::get().stop_measure();
            }
        }
    }

    // distance, min_value_in_array
    inline void calculate_children(const NodePtrType& node, const std::string& query) {

        const std::size_t current_index_shift = trie.get_index(node) * (query.size() + 1);

        ChildPtrIteratorType it;
        ChildPtrIteratorType end;
        std::tie(it, end) = trie.get_child_iterator(node);

        for (; it != end; it++) {
            NodePtrType child = trie.dereference_child_iterator(it);

            const char character = trie.get_character(child);
            const float insert_penalty = penalty.insert(character);
            const std::size_t child_index_shift = trie.get_index(child) * (query.size() + 1);

            float min = dp[child_index_shift] = dp[current_index_shift] + insert_penalty;

            for (std::size_t i = 1; i <= query.size(); i++) {
                dp[child_index_shift + i] =
                    std::min(std::min(dp[current_index_shift + i] + insert_penalty,
                                      dp[child_index_shift + i - 1] + penalty.remove(query[i - 1])),
                             dp[current_index_shift + i - 1] + penalty.modify(character, query[i - 1]));
                if constexpr (early_break)
                    min = std::min(min, dp[child_index_shift + i]);
            }

            trie.get_payload(child).min_distance = min;
            trie.get_payload(child).distance = dp[child_index_shift + query.size()];
        }
    }

    inline bool has_children(NodePtrType node) {
        auto iters = trie.get_child_iterator(node);
        return iters.first != iters.second;
    }

    std::vector<std::pair<float, std::string>> query(const std::string& query, const std::size_t n) noexcept {
        // Prepare

        if constexpr (collect_stats) {
            statistics_collector::get().add_stat("num_nodes", std::to_string(trie.get_num_nodes()));
        }
        std::size_t skipped_nodes = 0;

        std::queue<NodePtrType> task_queue;
        std::mutex global_task_queue_mtx;
        const std::size_t query_size = query.size();

        dp.resize(trie.get_num_nodes() * (query_size + 1));

        dp[0] = 0;
        early_break_min_max.store(std::numeric_limits<float>().max(), std::memory_order_relaxed);

        for (std::size_t i = 1; i <= query_size; i++) {
            dp[i] = dp[i - 1] + penalty.remove(query[i - 1]);
        }

        calculate_children(trie.get_root(), query);

        ChildPtrIteratorType begin;
        ChildPtrIteratorType end;
        std::tie(begin, end) = trie.get_child_iterator(trie.get_root());
        for (ChildPtrIteratorType it = begin; it != end; it++) {
            task_queue.push(trie.dereference_child_iterator(it));
        }

        // Parallel execution

        std::mutex global_queue_mtx;
        std::priority_queue<std::pair<float, NodePtrType>> q;
        std::vector<std::thread> threads;
        struct alignas(CACHE_LINE_SIZE / 2) Signal {
            Signal() : other_needes_work(false) {}
            std::atomic<bool> other_needes_work;
        };
        Signal signals[num_threads];
        std::atomic<std::size_t> global_missing(trie.get_num_nodes() - 1);

        for (unsigned int t = 0; t < num_threads; t++) {
            threads.emplace_back([&, t]() {
                std::priority_queue<std::pair<float, NodePtrType>> local_q;
                std::queue<NodePtrType> local_task_queue;

                // Work from Queue until all nodes are processed
                NodePtrType current;
                std::size_t local_done = 0;
                ChildPtrIteratorType it;
                ChildPtrIteratorType end;
                float global_min_max = early_break_min_max.load(std::memory_order_relaxed);
                std::size_t local_skipped = 0;

                while (true) {
                    global_task_queue_mtx.lock();
                    if (!task_queue.empty()) {

                        local_task_queue.push(task_queue.front());
                        task_queue.pop();

                        global_task_queue_mtx.unlock();

                        // got work
                        do {
                            current = local_task_queue.front();
                            local_task_queue.pop();

                            calculate_children(current, query);
                            local_done++;

                            // Fill Task-Queue and update current
                            std::tie(it, end) = trie.get_child_iterator(current);

                            // Check the local minimum only once for all
                            // children
                            if constexpr (early_break) {
                                if (local_q.size() >= n) {
                                    global_min_max = early_break_min_max.load(std::memory_order_relaxed);

                                    if (local_q.top().first < global_min_max) {
                                        early_break_min_max.store(local_q.top().first, std::memory_order_relaxed);
                                        global_min_max = local_q.top().first;
                                    }
                                }
                            }

                            // Add children to work and result queue
                            for (; it != end; it++) {
                                NodePtrType child = trie.dereference_child_iterator(it);

                                // Work -> Local Queue
                                if (trie.is_leaf(child)) {
                                    if (local_q.size() < n ||
                                        trie.get_payload(child).distance < local_q.top().first) { // order matters here
                                        local_q.emplace(trie.get_payload(child).distance, child);
                                        if (local_q.size() > n)
                                            local_q.pop();
                                    }
                                }

                                // Test if we should check out children of
                                // children
                                if constexpr (early_break) {
                                    // Do not explore if it can only get worse
                                    if (local_q.size() >= n) {
                                        if (trie.get_payload(child).min_distance >
                                            std::min(global_min_max, local_q.top().first)) {
                                            local_done += trie.get_payload(child).num_children + 1;

                                            if constexpr (collect_stats) {
                                                local_skipped += trie.get_payload(child).num_children + 1;
                                            }

                                        } else if (has_children(child)) {
                                            // We keep one child locally
                                            local_task_queue.push(child);
                                        } else {
                                            // does not have children, skip
                                            // queuing
                                            local_done++;
                                        }
                                    } else {
                                        local_task_queue.push(child);
                                    }
                                } else {
                                    local_task_queue.push(child);
                                }
                            }

                            const std::size_t size = local_task_queue.size();
                            if (size > 1000 && signals[t].other_needes_work.load(std::memory_order_relaxed)) {
                                global_task_queue_mtx.lock();

                                for (std::size_t i = 0; i < size / 2; i++) {
                                    task_queue.push(local_task_queue.front());
                                    local_task_queue.pop();
                                }

                                // std::cout << task_queue.size() << std::endl;
                                global_task_queue_mtx.unlock();
                                signals[t].other_needes_work.store(false, std::memory_order_relaxed);
                            }
                        } while (!local_task_queue.empty());

                    } else { // global queue is empty
                        global_task_queue_mtx.unlock();
                        // update done counter and check if thread should stop
                        if (local_done) {
                            std::size_t missing =
                                global_missing.fetch_sub(local_done, std::memory_order_relaxed) - local_done;

                            if (missing <= 0) {
                                break;
                            }
                            local_done = 0;
                        } else if (global_missing.load(std::memory_order_relaxed) <= 0) {
                            break;
                        }

                        signals[(t + 1) % num_threads].other_needes_work.store(true, std::memory_order_relaxed);
                    }
                }

                // Local Queue -> Global Queue
                global_queue_mtx.lock();
                while (!local_q.empty()) {
                    std::pair<float, NodePtrType> entry = std::move(local_q.top());
                    local_q.pop();

                    if (q.size() < n || entry.first < q.top().first) { // order matters here
                        q.push(entry);
                        if (q.size() > n)
                            q.pop();
                    }
                }
                skipped_nodes += local_skipped;
                global_queue_mtx.unlock();
                //-------------------------------
            });
        }

        // Wait for threads to finish execution
        for (auto& thread : threads)
            thread.join();

        // Queue -> Result vector
        std::vector<std::pair<float, std::string>> result(q.size());
        while (!q.empty()) {
            result[q.size() - 1] = std::make_pair(q.top().first, trie.get_word(q.top().second));
            q.pop();
        }

        if constexpr (collect_stats) {
            statistics_collector::get().add_stat("skipped_nodes", std::to_string(skipped_nodes));
        }

        return result;
    }

  private:
    alignas(CACHE_LINE_SIZE) const PenaltyClass penalty;
    alignas(CACHE_LINE_SIZE) const std::size_t num_threads;
    alignas(CACHE_LINE_SIZE) ParallelTrieImpl trie;
    alignas(CACHE_LINE_SIZE) std::vector<float> dp;
    // the smallest of all largest elements in queues
    alignas(CACHE_LINE_SIZE) std::atomic<float> early_break_min_max;
};
