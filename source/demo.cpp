#include "implementation/accelerated_levenshtein.hpp"
#include "implementation/accelerated_levenshtein_sequential.hpp"
#include "implementation/naive_levenshtein.hpp"
#include "implementation/sequential_levenshtein.hpp"
#include "implementation/sequential_trie.hpp"
#include "utils/commandline.h"
#include "utils/line_reader.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    CommandLine cl(argc, argv);

    std::string lang = cl.strArg("-lang", "de");
    std::vector<std::string> words;
    std::string penalty_path;

    if (lang == "de") {
        LineReader reader("../data/german_words.txt");
        words = reader.read();
        penalty_path = "../data/weights_german.txt";

    } else if (lang == "en") {
        LineReader reader("../data/english_words.txt");
        words = reader.read();
        penalty_path = "../data/weights_english.txt";

    } else {
        std::cout << "unknown lang" << std::endl;
        return 1;
    }

    levenshtein::KBDistance penalty(penalty_path);
    AcceleratedLevenshtein acc_lev(penalty, 8);
    NaiveLevenshtein naive_lev(penalty, 8);

    auto t0 = std::chrono::high_resolution_clock::now();
    acc_lev.precompute(words);
    auto t1 = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.;
    std::cout << "accelerated (trie) precompute took: " << duration << " ms" << std::endl;

    t0 = std::chrono::high_resolution_clock::now();
    naive_lev.precompute(words);
    t1 = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.;
    std::cout << "naive parallel-for precompute took: " << duration << " ms" << std::endl;

    std::string query;
    while (std::cin >> query) {

        std::cout << "acc:" << std::endl;

        double duration = 0;
        std::vector<std::pair<float, std::string>> results;

        for (int i = 0; i < 50; i++) {
            auto t0 = std::chrono::high_resolution_clock::now();
            results = acc_lev.query(query, 10);
            auto t1 = std::chrono::high_resolution_clock::now();
            duration += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.;
        }

        for (auto line : results) {
            std::cout << std::fixed << std::setprecision(3) << line.first << " " << line.second << std::endl;
        }

        std::cout << "Accelerated (trie) took: " << duration / 50 << " ms\n";

        duration = 0;

        std::cout << "naive:" << std::endl;

        for (int i = 0; i < 10; i++) {
            t0 = std::chrono::high_resolution_clock::now();
            results = naive_lev.query(query, 10);
            t1 = std::chrono::high_resolution_clock::now();

            duration += std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() / 1000.;
        }

        for (auto line : results) {
            std::cout << std::fixed << std::setprecision(3) << line.first << " " << line.second << std::endl;
        }

        std::cout << "Naive parallel-for took: " << duration / 10 << " ms\n";
    }

    return 0;
}
