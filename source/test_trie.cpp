#include "source/common.hpp"
#include "utils/line_reader.hpp"

#include <random>

int main() {
    bool all_passed = true;

    for_each_in_tuple(trie_impls, [&](const auto& x) {
        std::cout << "Testing " << x.name << "..." << std::flush;
        bool passed = true;

        LineReader reader("../data/german_words.txt");
        std::vector<std::string> words = reader.read();

        auto rng = std::default_random_engine{};
        std::shuffle(words.begin(), words.end(), rng);

        auto trie = x.make(std::thread::hardware_concurrency());
        trie.insert(words);

        for (auto& word : words) {

            auto current = trie.get_root();

            for (char c : word) {

                auto child_iters = trie.get_child_iterator(current);
                auto next = child_iters.second;

                for (auto it = child_iters.first; it != child_iters.second; it++) {
                    if (trie.get_character(trie.dereference_child_iterator(it)) == c) {
                        next = it;
                    }
                }

                if (next == child_iters.second) {
                    passed = false;
                    all_passed = false;
                    break;
                }

                current = trie.dereference_child_iterator(next);
            }

            if (!trie.is_leaf(current)) {
                passed = false;
                all_passed = false;
                break;
            }
        }

        if (passed) {
            std::cout << "passed" << std::endl;
        } else {
            std::cout << "FAIL" << std::endl;
        }
    });

    if (all_passed)
        return 0;
    return 1;
}