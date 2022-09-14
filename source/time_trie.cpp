#include "utils/commandline.h"
#include "utils/line_reader.hpp"
#include "utils/statistics_collector.hpp"
#include "utils/string_utils.hpp"

#include "common.hpp"

#include <random>
#include <string>
#include <thread>

// --------- HELPER --------------

template <class TrieImpl> std::size_t compute_number_children(TrieImpl& trie, typename TrieImpl::NodePtrType ptr) {
    typename TrieImpl::ChildPtrIteratorType it;
    typename TrieImpl::ChildPtrIteratorType end;

    std::tie(it, end) = trie.get_child_iterator(ptr);

    if (it == end)
        return 0;

    std::size_t count = 0;

    for (; it != end; it++) {
        count += compute_number_children(trie, trie.dereference_child_iterator(it)) + 1;
    }

    return count;
}

// --------- TESTS ---------------

template <class TrieImpl> void test_insert(TrieImpl& trie, std::vector<std::string>& words) {
    trie.insert(words);
}

template <class TrieImpl> void test_bfs(TrieImpl& trie, std::size_t num_threads) {
    bfs_trie<TrieImpl>(
        trie, trie.get_root(),
        []([[maybe_unused]] TrieImpl& l_trie, [[maybe_unused]] typename TrieImpl::NodePtrType node) {
            // do nothing...
        },
        num_threads);
}

template <class TrieImpl> void test_count_children(TrieImpl& trie) {
    compute_number_children(trie, trie.get_root());
}

template <class TrieImpl> void test_extract_words(TrieImpl& trie, std::size_t num_threads) {
    bfs_trie<TrieImpl>(
        trie, trie.get_root(),
        [](TrieImpl& l_trie, typename TrieImpl::NodePtrType node) {
            if (l_trie.is_leaf(node)) {
                l_trie.get_word(node);
            }
        },
        num_threads);
}

// --------- DRIVER --------------

int main(int argn, char** argc) {

    const std::string datasets[] = {"../data/german_words.txt", "../data/english_words.txt"};
    const std::string dataset_short[] = {"dic_de", "dic_en"};
    const std::size_t iterations = 5;
    const std::size_t partitions = 8;

    CommandLine cl(argn, argc);

    std::string output = cl.strArg("-o", "");

    if (output.empty()) {
        std::cout << "Specify output!" << std::endl;
        return 1;
    }

    std::ofstream file(output);

    print(file, "index", 5);
    print(file, "dataset", 8);
    print(file, "words", 9);
    print(file, "#it", 3);
    print(file, "threads", 8);
    print(file, "impl", 40);
    print(file, "test", 20);
    print(file, "type", 10);
    print(file, "key", 50);
    print(file, "value", 10);
    print(file, "\n");

    int index = 0;
    int dataset_short_index = 0;
    for (auto& dataset : datasets) {
        LineReader reader(dataset);
        std::vector<std::string> words = reader.read();

        std::sort(words.begin(), words.end());
        words.erase(std::unique(words.begin(), words.end()), words.end());

        auto rng = std::default_random_engine{};
        std::shuffle(words.begin(), words.end(), rng);

        std::vector<std::string> partition;
        std::size_t partition_size = (words.size() + partitions - 1) / partitions;

        for (std::size_t i = 0; i < partitions; i++) {

            std::cout << "Testing " << dataset << " partition " << i + 1 << "/" << partitions << std::endl;

            // update partition vector
            for (std::size_t j = i * partition_size; j < std::min((i + 1) * partition_size, words.size()); j++)
                partition.push_back(words[j]);

            for (std::size_t it = 0; it < iterations; it++) {

                for_each_in_tuple(trie_impls, [&](const auto& x) {
                    std::vector<std::tuple<std::function<void(std::size_t)>, bool, std::string>>
                        tests; // test, multithreaded, name

                    // ADD TESTS
                    tests.push_back({[&]([[maybe_unused]] std::size_t threads) {
                                         statistics_collector::get().start_measure("total");

                                         statistics_collector::get().start_measure("construct");
                                         auto trie = x.make(threads);
                                         statistics_collector::get().stop_measure();

                                         statistics_collector::get().start_measure("insert");
                                         test_insert(trie, partition);
                                         statistics_collector::get().stop_measure();

                                         statistics_collector::get().stop_measure();
                                     },
                                     true, "t_insert"});

                    auto trie = x.make(std::thread::hardware_concurrency());
                    trie.insert(partition);

                    tests.push_back({[&](std::size_t threads) {
                                         statistics_collector::get().start_measure("total");
                                         test_bfs(trie, threads);
                                         statistics_collector::get().stop_measure();
                                     },
                                     true, "t_bfs"});

                    tests.push_back({[&]([[maybe_unused]] std::size_t threads) {
                                         statistics_collector::get().start_measure("total");
                                         test_count_children(trie);
                                         statistics_collector::get().stop_measure();
                                     },
                                     false, "t_count_children"});

                    tests.push_back({[&](std::size_t threads) {
                                         statistics_collector::get().start_measure("total");
                                         test_extract_words(trie, threads);
                                         statistics_collector::get().stop_measure();
                                     },
                                     true, "t_extract_words"});

                    for (auto& test : tests) {

                        std::size_t min_threads =
                            !std::get<1>(test) ? 1 : (partitions - 1 != i ? std::thread::hardware_concurrency() : 1);
                        std::size_t max_threads = std::get<1>(test) ? std::thread::hardware_concurrency() : 1;

                        for (std::size_t threads = min_threads; threads <= max_threads; threads++) {

                            statistics_collector::get().reset();
                            std::get<0>(test)(threads);

                            for (auto time : statistics_collector::get().get_times()) {
                                print(file, index, 5);
                                print(file, dataset_short[dataset_short_index], 8);
                                print(file, partition.size(), 9);
                                print(file, it, 3);
                                print(file, threads, 8);
                                print(file, x.name, 40);
                                print(file, std::get<2>(test), 20);
                                print(file, "time", 10);
                                print(file, time.first, 50);
                                print(file, time.second, 10);
                                print(file, "\n");
                            }

                            for (auto statistic : statistics_collector::get().get_stats()) {
                                print(file, index, 5);
                                print(file, dataset_short[dataset_short_index], 8);
                                print(file, partition.size(), 9);
                                print(file, it, 3);
                                print(file, threads, 8);
                                print(file, x.name, 40);
                                print(file, std::get<2>(test), 20);
                                print(file, "statistic", 10);
                                print(file, statistic.first, 50);
                                print(file, statistic.second, 10);
                                print(file, "\n");
                            }

                            index++;
                        }
                    }
                });
            }
        }

        dataset_short_index++;
    }

    print(file, "\n");
    return 0;
}
