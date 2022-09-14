#pragma once

#include "implementation/trie.hpp"
#include "utils/statistics_collector.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <vector>

#include <boost/sort/sort.hpp>

template <class PayloadType = DummyPayload, class TrieImpl = ParallelTrie<PayloadType>, bool collect_stats = false>
class SortedBuildupTrie {

  public:
    SortedBuildupTrie(std::size_t num_threads = std::thread::hardware_concurrency())
        : num_threads(num_threads), trie(num_threads), null(trie.null) {}

  public:
    using NodePtrType = typename TrieImpl::NodePtrType;
    using ChildPtrIteratorType = typename TrieImpl::ChildPtrIteratorType;

    void insert(std::vector<std::string>& words) {
        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("copy");
        }
        std::vector<std::string> sorted_words = words;
        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }

        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("sort");
        }
        boost::sort::sample_sort(sorted_words.begin(), sorted_words.end(), num_threads);
        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }

        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("insert");
        }
        trie.insert(sorted_words);
        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }
    }

    NodePtrType get_root() {
        return trie.get_root();
    }

    std::size_t get_num_nodes() {
        return trie.get_num_nodes();
    }

    // It must be guaranteed that children have subsequent indices!
    std::size_t get_index(NodePtrType node_ptr) {
        return trie.get_index(node_ptr);
    }

    bool is_leaf(NodePtrType node_ptr) {
        return trie.is_leaf(node_ptr);
    }

    NodePtrType get_parent(NodePtrType node_ptr) {
        return trie.get_parent(node_ptr);
    }

    char get_character(NodePtrType node_ptr) {
        return trie.get_character(node_ptr);
    }

    PayloadType& get_payload(NodePtrType node_ptr) {
        return trie.get_payload(node_ptr);
    }

    std::pair<ChildPtrIteratorType, ChildPtrIteratorType> get_child_iterator(NodePtrType node_ptr) {
        return trie.get_child_iterator(node_ptr);
    }

    NodePtrType dereference_child_iterator(ChildPtrIteratorType it) {
        return trie.dereference_child_iterator(it);
    }

    std::string get_word(NodePtrType node_ptr) {
        return trie.get_word(node_ptr);
    }

  private:
    std::size_t num_threads;
    TrieImpl trie;

  public:
    NodePtrType null;
};
