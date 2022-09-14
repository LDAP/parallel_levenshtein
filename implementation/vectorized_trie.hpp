#pragma once

#include "implementation/concurrent_container.hpp"
#include "implementation/trie.hpp"
#include "utils/statistics_collector.hpp"

#include <atomic>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <thread>
#include <vector>

template <class PayloadType = DummyPayload, bool collect_stats = false> class VectorizedParallelTrie {

  public:
    struct VectorizedParallelTrieNode {
        friend VectorizedParallelTrie;

      private:
        bool leaf;
        // must be 0 for root
        std::size_t parent;
        char character;
        PayloadType payload;

        // compress the static array into this vector
        std::size_t children_begin_index;
        std::size_t children_end_index;
    };

  private:
    using HelperNodePtrType = typename ParallelTrie<DummyPayload>::NodePtrType;
    using NodeType = VectorizedParallelTrieNode;

  public:
    using NodePtrType = std::size_t;
    using ChildPtrIteratorType = std::size_t;
    static const NodePtrType null = 0;

  public:
    VectorizedParallelTrie(std::size_t num_threads = std::thread::hardware_concurrency())
        : num_threads(num_threads), num_nodes(1) {}

    void insert(std::vector<std::string>& words) {
        ParallelTrie helper(num_threads);
        helper.insert(words);

        nodes.resize(helper.get_num_nodes());
        num_nodes = helper.get_num_nodes();

        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("vectorize");
        }

        bfs_trie<ParallelTrie<DummyPayload>>(
            helper, helper.get_root(),
            [&]([[maybe_unused]] auto& _helper, auto node_ptr) {
                std::size_t current_index = helper.get_index(node_ptr);

                nodes[current_index].leaf = helper.is_leaf(node_ptr);
                nodes[current_index].parent =
                    helper.get_parent(node_ptr) ? helper.get_index(helper.get_parent(node_ptr)) : 0;
                nodes[current_index].character = helper.get_character(node_ptr);

                auto iters = helper.get_child_iterator(node_ptr);

                if (iters.first != iters.second) {
                    std::size_t min_index = std::numeric_limits<std::size_t>().max();
                    std::size_t max_index = 0;

                    for (auto it = iters.first; it != iters.second; it++) {
                        std::size_t child_index = helper.get_index(helper.dereference_child_iterator(it));
                        min_index = std::min(min_index, child_index);
                        max_index = std::max(max_index, child_index);
                    }

                    nodes[current_index].children_begin_index = min_index;
                    nodes[current_index].children_end_index = max_index + 1;
                } else {
                    nodes[current_index].children_begin_index = 0;
                    nodes[current_index].children_end_index = 0;
                }
            },
            num_threads);

        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }
    }

    NodePtrType get_root() {
        return 0;
    }

    std::size_t get_num_nodes() {
        return num_nodes;
    }

    std::size_t get_index(NodePtrType node_ptr) {
        return node_ptr;
    }

    bool is_leaf(NodePtrType node_ptr) {
        return nodes[node_ptr].leaf;
    }

    NodePtrType get_parent(NodePtrType node_ptr) {
        return nodes[node_ptr].parent;
    }

    char get_character(NodePtrType node_ptr) {
        return nodes[node_ptr].character;
    }

    PayloadType& get_payload(NodePtrType node_ptr) {
        return nodes[node_ptr].payload;
    }

    std::pair<ChildPtrIteratorType, ChildPtrIteratorType> get_child_iterator(NodePtrType node_ptr) {
        return {nodes[node_ptr].children_begin_index, nodes[node_ptr].children_end_index};
    }

    NodePtrType dereference_child_iterator(ChildPtrIteratorType it) {
        return it;
    }

    std::string get_word(NodePtrType node_ptr) {
        std::string result = "";

        NodePtrType current = node_ptr;

        do {
            result = nodes[current].character + result;
            current = nodes[current].parent;
        } while (current);

        return result;
    }

  private:
    std::vector<NodeType> nodes;
    std::size_t num_threads;
    std::size_t num_nodes;
};
