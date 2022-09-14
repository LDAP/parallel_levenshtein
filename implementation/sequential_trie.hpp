#pragma once

#include "implementation/concurrent_container.hpp"
#include "implementation/trie.hpp"

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

template <class PayloadType = DummyPayload> class SequentialTrie {

  public:
    struct SequentialTrieNode {
        friend SequentialTrie;

        static constexpr int CHAR_SIZE = 128;

        SequentialTrieNode(SequentialTrieNode* parent, char character)
            : leaf(false), parent(parent), character(character), children(CHAR_SIZE) {
            for (int i = 0; i < CHAR_SIZE; i++)
                children[i] = nullptr;
        }

        ~SequentialTrieNode() {
            for (SequentialTrieNode* child : children) {
                delete child;
            }
        }

      private:
        // can only be changed to true -> no atomic needed
        bool leaf;
        // must be 0 for root
        SequentialTrieNode* const parent;
        const char character;
        PayloadType payload;

        // may be deleted, and set to nullptr
        std::vector<SequentialTrieNode*> children;
        std::size_t index;
    };

  private:
    using NodeType = SequentialTrieNode;

  public:
    using NodePtrType = NodeType*;
    using ChildPtrIteratorType = typename std::vector<SequentialTrieNode*>::const_iterator;
    NodePtrType null = nullptr;

  public:
    SequentialTrie() : root(new SequentialTrieNode(nullptr, '\0')), num_nodes(1) {}

    SequentialTrie([[maybe_unused]] std::size_t num_threads) : SequentialTrie() {}

    ~SequentialTrie() {
        delete root;
    }

    void insert(std::vector<std::string>& words) {

        // BUILD TRIE -----------------------------
        for (std::string& word : words) {
            NodeType* current = root;

            for (std::size_t j = 0; j < word.size(); j++) {

                NodePtrType& child = current->children[static_cast<int>(word[j])];

                // maybe a string containes special characters that do not
                // fit in out datatype
                assert(word[j] < NodeType::CHAR_SIZE && word[j] > 0);
                // std::cout << word << word[j] << " " <<
                // static_cast<int>(word[j]) << "\n";

                if (child == nullptr) {
                    child = new SequentialTrieNode(current, word[j]);
                    num_nodes++;
                }

                current = child;
            }

            current->leaf = true;
        }

        // COMPRESS CHILDREN -----------------------
        // AND SET INDICES -------------------------

        std::queue<NodePtrType> q;
        q.push(root);

        std::size_t index_ctr = 0;
        while (!q.empty()) {
            NodePtrType current = q.front();
            q.pop();

            // set index
            current->index = index_ctr++;

            // compress children
            std::vector<NodePtrType> compressed_children;
            for (std::size_t i = 0; i < current->children.size(); i++) {
                if (current->children[i] != nullptr) {
                    compressed_children.emplace_back(current->children[i]);
                    q.push(current->children[i]);
                }
            }

            current->children = std::move(compressed_children);
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
