#pragma once

#include "implementation/concurrent_container.hpp"
#include "utils/statistics_collector.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <thread>
#include <vector>

template <class TrieImpl>
inline void bfs_trie(TrieImpl& trie, typename TrieImpl::NodePtrType root,
                     const std::function<void(TrieImpl& trie, typename TrieImpl::NodePtrType)>& for_each_node,
                     std::size_t num_threads = std::thread::hardware_concurrency()) {
    using ChildPtrIteratorType = typename TrieImpl::ChildPtrIteratorType;
    using NodePtrType = typename TrieImpl::NodePtrType;

    // EXPLORE ROOT
    for_each_node(trie, root);

    // PREPARE TASK QUEUE
    std::mutex global_task_queue_mtx;
    std::queue<NodePtrType> task_queue;

    ChildPtrIteratorType begin;
    ChildPtrIteratorType end;
    std::tie(begin, end) = trie.get_child_iterator(trie.get_root());

    for (ChildPtrIteratorType it = begin; it != end; it++) {
        task_queue.push(trie.dereference_child_iterator(it));
    }

    std::vector<std::thread> threads;

    struct alignas(CACHE_LINE_SIZE / 2) Signal {
        Signal() : other_needes_work(false) {}

        std::atomic<bool> other_needes_work;
    };
    Signal signals[num_threads];

    std::atomic<std::size_t> global_missing(trie.get_num_nodes() - 1);

    // PARALLEL EXECUTION
    for (unsigned int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            std::queue<NodePtrType> local_task_queue;

            // Work from Queue until all nodes are processed
            NodePtrType current;
            std::size_t local_done = 0;
            ChildPtrIteratorType it;
            ChildPtrIteratorType end;

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

                        for_each_node(trie, current);
                        local_done++;

                        // Fill Task-Queues
                        std::tie(it, end) = trie.get_child_iterator(current);

                        for (; it != end; it++) {
                            local_task_queue.push(trie.dereference_child_iterator(it));
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
            //-------------------------------
        });
    }

    // Wait for threads to finish execution
    for (auto& thread : threads)
        thread.join();
}

struct DummyPayload {};

template <class PayloadType = DummyPayload, bool collect_stats = false> class ParallelTrie {

  public:
    // Evaluation shows that is is better to have a separate node with vectors
    struct ParallelTrieNode {
        friend ParallelTrie;

        static constexpr int CHAR_SIZE = 128;

        ParallelTrieNode(ParallelTrieNode* parent, char character)
            : leaf(false), parent(parent), character(character) {}

        ~ParallelTrieNode() {
            for (ParallelTrieNode* child : children)
                delete child;
        }

      private:
        // can only be changed to true -> no atomic needed
        bool leaf;
        // must be 0 for root
        ParallelTrieNode* const parent;
        const char character;
        PayloadType payload;

        // compress the static array into this vector
        std::vector<ParallelTrieNode*> children;
        std::size_t index;
    };

  private:
    struct TempTrieNode {
        friend ParallelTrie;

        static constexpr int CHAR_SIZE = 128;

        TempTrieNode(TempTrieNode* parent, char character) : leaf(false), parent(parent), character(character) {

            for (int i = 0; i < CHAR_SIZE; i++)
                children[i].store(nullptr, std::memory_order_relaxed);
        }

        ~TempTrieNode() {
            for (int i = 0; i < CHAR_SIZE; i++)
                if (children[i].load(std::memory_order_relaxed) != nullptr)
                    delete children[i].load();
        }

      private:
        // can only be changed to true -> no atomic needed
        bool leaf;
        // must be 0 for root
        TempTrieNode* const parent;
        ParallelTrieNode* brother;
        const char character;
        // may be deleted, and set to nullptr
        std::atomic<TempTrieNode*> children[CHAR_SIZE];
    };

    using NodeType = ParallelTrieNode;

  public:
    using NodePtrType = NodeType*;
    using ChildPtrIteratorType = typename std::vector<ParallelTrieNode*>::const_iterator;
    NodePtrType null = nullptr;

  public:
    ParallelTrie(std::size_t num_threads = std::thread::hardware_concurrency())
        : root(new ParallelTrieNode(nullptr, '\0')), num_threads(num_threads), num_nodes(1) {}

    ~ParallelTrie() {
        delete root;
    }

    void insert(std::vector<std::string>& words) {

        if constexpr (collect_stats) {
            statistics_collector::get().start_measure("build");
        }

        // BUILD TRIE -----------------------------
        TempTrieNode* temp_root = new TempTrieNode(nullptr, '\0');
        std::size_t collisions = 0;

#pragma omp parallel num_threads(num_threads), shared(num_nodes, collisions)
        {
            std::size_t local_nodes = 0;
            std::size_t local_collisions = 0;

#pragma omp for
            for (std::size_t i = 0; i < words.size(); i++) {
                std::string& word = words[i];

                TempTrieNode* current = temp_root;

                for (std::size_t j = 0; j < word.size(); j++) {

                    // maybe a string containes special characters that does not
                    // fit in our datatype
                    assert(word[j] < TempTrieNode::CHAR_SIZE && word[j] > 0);

                    std::atomic<TempTrieNode*>& child = current->children[static_cast<int>(word[j])];

                    TempTrieNode* child_ptr = child.load(std::memory_order_relaxed);

                    if (child_ptr == nullptr) {
                        TempTrieNode* new_child_ptr = new TempTrieNode(current, word[j]);
                        if (child.compare_exchange_strong(child_ptr, new_child_ptr, std::memory_order_relaxed)) {
                            current = new_child_ptr;
                            local_nodes++;
                        } else {
                            delete new_child_ptr;
                            current = child_ptr;
                            if constexpr (collect_stats)
                                local_collisions++;
                        }
                    } else {
                        current = child_ptr;
                    }
                }

                current->leaf = true;
            }

#pragma omp atomic
            num_nodes += local_nodes;
            if constexpr (collect_stats) {
#pragma omp atomic
                collisions += local_collisions;
            }
        }

        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
            statistics_collector::get().add_stat("collisions", std::to_string(collisions));
            statistics_collector::get().start_measure("compress");
        }

        // COMPRESS CHILDREN -----------------------------

        // PREPARE TASK QUEUE
        std::mutex global_task_queue_mtx;
        std::queue<TempTrieNode*> task_queue;

        ChildPtrIteratorType begin;
        ChildPtrIteratorType end;
        // COPY NODE
        temp_root->brother = root;
        temp_root->brother->leaf = temp_root->leaf;

        // ADD CHILDREN
        for (int i = 0; i < ParallelTrieNode::CHAR_SIZE; i++) {
            TempTrieNode* child = temp_root->children[i].load(std::memory_order_relaxed);
            if (child != nullptr) {
                ParallelTrieNode* new_child = new ParallelTrieNode(temp_root->brother, child->character);
                child->brother = new_child;
                temp_root->brother->children.push_back(new_child);
                task_queue.push(child);
            }
        }

        std::vector<std::thread> threads;

        struct alignas(CACHE_LINE_SIZE / 2) Signal {
            Signal() : other_needes_work(false) {}

            std::atomic<bool> other_needes_work;
        };
        Signal signals[num_threads];

        std::atomic<std::size_t> global_missing(num_nodes - 1);

        // PARALLEL EXECUTION
        for (unsigned int t = 0; t < num_threads; t++) {
            threads.emplace_back([&, t]() {
                std::queue<TempTrieNode*> local_task_queue;

                // Work from Queue until all nodes are processed
                TempTrieNode* current;
                std::size_t local_done = 0;
                ChildPtrIteratorType it;
                ChildPtrIteratorType end;

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

                            // COPY NODE
                            current->brother->leaf = current->leaf;

                            // ADD CHILDREN
                            for (int i = 0; i < ParallelTrieNode::CHAR_SIZE; i++) {
                                TempTrieNode* child = current->children[i].load(std::memory_order_relaxed);
                                if (child != nullptr) {
                                    ParallelTrieNode* new_child =
                                        new ParallelTrieNode(current->brother, child->character);
                                    child->brother = new_child;
                                    current->brother->children.push_back(new_child);
                                    local_task_queue.push(child);
                                }
                            }

                            local_done++;

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
                //-------------------------------
            });
        }

        // Wait for threads to finish execution
        for (auto& thread : threads)
            thread.join();

        delete temp_root;

        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
            statistics_collector::get().start_measure("set_indices");
        }

        // SET INDICES -----------------------------

        std::queue<NodePtrType> q;
        q.push(root);

        std::size_t index_ctr = 0;
        while (!q.empty()) {
            NodePtrType current = q.front();
            q.pop();

            current->index = index_ctr++;
            for (auto child : current->children)
                q.push(child);
        }

        if constexpr (collect_stats) {
            statistics_collector::get().stop_measure();
        }
    }

    NodePtrType get_root() {
        return root;
    }

    std::size_t get_num_nodes() {
        return num_nodes;
    }

    // It must be guaranteed that children have subsequent indices!
    std::size_t get_index(NodePtrType node_ptr) {
        return node_ptr->index;
    }

    bool is_leaf(NodePtrType node_ptr) {
        return node_ptr->leaf;
    }

    NodePtrType get_parent(NodePtrType node_ptr) {
        return node_ptr->parent;
    }

    char get_character(NodePtrType node_ptr) {
        return node_ptr->character;
    }

    PayloadType& get_payload(NodePtrType node_ptr) {
        return node_ptr->payload;
    }

    std::pair<ChildPtrIteratorType, ChildPtrIteratorType> get_child_iterator(NodePtrType node_ptr) {
        return {node_ptr->children.begin(), node_ptr->children.end()};
    }

    NodePtrType dereference_child_iterator(ChildPtrIteratorType it) {
        return *it;
    }

    std::string get_word(NodePtrType node_ptr) {
        std::string result = "";

        NodePtrType current = node_ptr;

        while (current->parent) {
            result = current->character + result;
            current = current->parent;
        }

        return result;
    }

  private:
    NodePtrType root;
    std::size_t num_threads;
    std::size_t num_nodes;
};
