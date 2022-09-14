#pragma once

#include "implementation/accelerated_levenshtein.hpp"
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

struct SeqLevTriePayload {
    float min_distance;
    float distance;
};

// set early break to true to skip sub-trees that cannot be better that the
// current best
template <class PenaltyClass = levenshtein::KBDistance,
          class ParallelTrieImpl = VectorizedParallelTrie<SeqLevTriePayload>, bool early_break = true,
          bool collect_stats = false>
class AcceleratedLevenshteinSeq {

  private:
    using NodePtrType = typename ParallelTrieImpl::NodePtrType;
    using ChildPtrIteratorType = typename ParallelTrieImpl::ChildPtrIteratorType;

  public:
    AcceleratedLevenshteinSeq(PenaltyClass penalty) : penalty(penalty), trie(std::thread::hardware_concurrency()) {}

    void precompute(std::vector<std::string>& words) noexcept {
        trie.insert(words);
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

        std::queue<NodePtrType> task_queue;
        const std::size_t query_size = query.size();

        dp.resize(trie.get_num_nodes() * (query_size + 1));

        dp[0] = 0;

        for (std::size_t i = 1; i <= query_size; i++) {
            dp[i] = dp[i - 1] + penalty.remove(query[i - 1]);
        }

        calculate_children(trie.get_root(), query);

        ChildPtrIteratorType it;
        ChildPtrIteratorType end;
        std::tie(it, end) = trie.get_child_iterator(trie.get_root());
        for (; it != end; it++) {
            NodePtrType child = trie.dereference_child_iterator(it);
            task_queue.push(child);
        }

        std::priority_queue<std::pair<float, NodePtrType>> q;

        while (!task_queue.empty()) {
            NodePtrType current = task_queue.front();
            task_queue.pop();

            calculate_children(current, query);

            // Fill Task-Queue and update current
            std::tie(it, end) = trie.get_child_iterator(current);

            // Add children to work and result queue
            for (; it != end; it++) {
                NodePtrType child = trie.dereference_child_iterator(it);

                // Work -> Local Queue
                if (trie.is_leaf(child)) {
                    if (q.size() < n || trie.get_payload(child).distance < q.top().first) { // order matters here
                        q.emplace(trie.get_payload(child).distance, child);
                        if (q.size() > n)
                            q.pop();
                    }
                }

                // Test if we should check out children of
                // children
                if constexpr (early_break) {
                    // Do not explore if it can only get worse
                    if (q.size() < n ||
                        (trie.get_payload(child).min_distance <= q.top().first && has_children(child))) {
                        task_queue.push(child);
                    }
                } else {
                    task_queue.push(child);
                }
            }
        }

        // Queue -> Result vector
        std::vector<std::pair<float, std::string>> result(q.size());
        while (!q.empty()) {
            result[q.size() - 1] = std::make_pair(q.top().first, trie.get_word(q.top().second));
            q.pop();
        }

        return result;
    }

  private:
    const PenaltyClass penalty;
    ParallelTrieImpl trie;
    std::vector<float> dp;
};
