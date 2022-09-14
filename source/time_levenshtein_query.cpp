#include "utils/commandline.h"
#include "utils/line_reader.hpp"
#include "utils/string_utils.hpp"

#include "common.hpp"

#include <random>
#include <string>
#include <thread>

std::string random_string(const int len) {
    static const std::string alphanum = "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % alphanum.size()];
    }

    return tmp_s;
}

// --------- DRIVER --------------

int main(int argn, char** argc) {
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
    print(file, "n", 8);
    print(file, "query_size", 8);
    print(file, "query_type", 8);
    print(file, "query_idx", 8);
    print(file, "impl", 40);
    print(file, "type", 10);
    print(file, "key", 50);
    print(file, "value", 10);
    print(file, "\n");

    // DEFINE TEST SET
    // RUN EVERY TIME
    const std::string datasets[] = {"../data/german_words.txt", "../data/english_words.txt"};
    const std::string dataset_penalty_path[] = {"../data/weights_german.txt", "../data/weights_english.txt"};
    const std::string dataset_short[] = {"dic_de", "dic_en"};
    const std::size_t iterations = 5;

    const std::size_t random_queries = 25;
    const std::size_t queries_of_existing_dic_words = 25;
    const std::size_t queries_of_erased_dic_words = 25;

    // ONLY VARY ONE
    const std::size_t partitions = 8;
    const std::size_t ns[] = {1, 2, 3, 5, 10, 20, 50, 100, 1000};
    const std::size_t default_n = 10;

    // RUN TESTS

    std::size_t index = 0;
    int dataset_index = 0;
    for (auto& dataset : datasets) {
        LineReader reader(dataset);
        std::vector<std::string> words = reader.read();

        std::sort(words.begin(), words.end());
        words.erase(std::unique(words.begin(), words.end()), words.end());

        auto rng = std::default_random_engine{};
        std::shuffle(words.begin(), words.end(), rng);

        // PREPARE QUERIES
        std::vector<std::string> query_set;
        std::vector<std::string> query_type;
        for (std::size_t i = 0; i < queries_of_erased_dic_words; i++) {
            query_set.push_back(words[words.size() - 1]);
            query_type.push_back("erased");
            words.pop_back();
        }

        for (std::size_t i = 0; i < queries_of_existing_dic_words; i++) {
            query_set.push_back(words[words.size() - 1 - i]);
            query_type.push_back("existing");
        }

        for (std::size_t i = 1; i <= random_queries; i++) {
            query_set.push_back(random_string((100 / random_queries) * i));
            query_type.push_back("random");
        }

        std::shuffle(words.begin(), words.end(), rng);

        levenshtein::KBDistance penalty(dataset_penalty_path[dataset_index]);

        std::vector<std::string> partition;
        std::size_t partition_size = (words.size() + partitions - 1) / partitions;

        for (std::size_t i = 0; i < partitions; i++) {

            std::cout << "Testing " << dataset << " partition " << i + 1 << "/" << partitions << std::endl;

            // update partition vector
            for (std::size_t j = i * partition_size; j < std::min((i + 1) * partition_size, words.size()); j++)
                partition.push_back(words[j]);

            for_each_in_tuple(query_levenshtein_impls, [&](const auto& x) {
                std::size_t min_threads = x.seq ? 1 : (partitions - 1 != i ? std::thread::hardware_concurrency() : 1);
                std::size_t max_threads = x.seq ? 1 : std::thread::hardware_concurrency();

                for (std::size_t threads = min_threads; threads <= max_threads; threads++) {
                    // SKIP uninteresting thread counts (save time)
                    if (threads > std::thread::hardware_concurrency() / 2 && threads % 2 == 1)
                        continue;

                    auto impl = x.make(threads, penalty);
                    impl.precompute(partition);

                    for (std::size_t query = 0; query < query_set.size(); query++) {

                        for (std::size_t n : ns) {

                            // SKIP uninteresting runs (save time)
                            if (n != default_n && partitions - 1 != i)
                                continue;

                            for (std::size_t it = 0; it < iterations; it++) {
                                // EXECUTE RUN

                                statistics_collector::get().reset();
                                statistics_collector::get().start_measure("total");
                                impl.query(query_set[query], n);
                                statistics_collector::get().stop_measure();

                                for (auto time : statistics_collector::get().get_times()) {
                                    print(file, index, 5);
                                    print(file, dataset_short[dataset_index], 8);
                                    print(file, partition.size(), 9);
                                    print(file, it, 3);
                                    print(file, threads, 8);
                                    print(file, n, 8);
                                    print(file, query_set[query].size(), 8);
                                    print(file, query_type[query], 8);
                                    print(file, query, 8);
                                    print(file, x.name, 40);
                                    print(file, "time", 10);
                                    print(file, time.first, 50);
                                    print(file, time.second, 10);
                                    print(file, "\n");
                                }

                                for (auto statistic : statistics_collector::get().get_stats()) {
                                    print(file, index, 5);
                                    print(file, dataset_short[dataset_index], 8);
                                    print(file, partition.size(), 9);
                                    print(file, it, 3);
                                    print(file, threads, 8);
                                    print(file, n, 8);
                                    print(file, query_set[query].size(), 8);
                                    print(file, query_type[query], 8);
                                    print(file, query, 8);
                                    print(file, x.name, 40);
                                    print(file, "statistic", 10);
                                    print(file, statistic.first, 50);
                                    print(file, statistic.second, 10);
                                    print(file, "\n");
                                }
                            }

                            index++;
                        }
                    }
                }
            });
        }

        dataset_index++;
    }

    print(file, "\n");
    return 0;
}
