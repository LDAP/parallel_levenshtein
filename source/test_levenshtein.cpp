#include "source/common.hpp"
#include "utils/line_reader.hpp"

#include <algorithm>
#include <map>
#include <random>
#include <vector>

int main() {
    const std::string exact_match_test = "Algorithmen";
    const std::string non_exact_match_test = "Akgorighmwn";
    const std::size_t test_count = 100;

    LineReader reader("../data/german_words.txt");
    std::vector<std::string> words = reader.read();

    // Remove duplicates
    std::sort(words.begin(), words.end());
    words.erase(std::unique(words.begin(), words.end()), words.end());

    auto rng = std::default_random_engine{};
    std::shuffle(words.begin(), words.end(), rng);
    levenshtein::KBDistance penalty("../data/weights_german.txt");

    SequentialLevenshtein<> seq_lev(penalty);
    seq_lev.precompute(words);
    auto compare_result = seq_lev.query(non_exact_match_test, test_count);

    std::sort(compare_result.begin(), compare_result.end());

    bool all_passed = true;
    for_each_in_tuple(all_levenshtein_impls, [&](const auto& x) {
        std::cout << "Testing " << x.name << "..." << std::flush;
        bool passed = true;
        std::string reason = "";

        auto lev = x.make(std::thread::hardware_concurrency(), penalty);
        lev.precompute(words);

        // TEST EXACT MATCH

        auto result = lev.query(exact_match_test, test_count);
        std::sort(result.begin(), result.end());

        if (result[0].second != exact_match_test) {
            passed = false;
            reason += "Exact match was not found.";
        }

        if (std::abs(result[0].first) >= 1e-6) {
            passed = false;
            reason += "Exact match must have penalty < 1e-6, was " + std::to_string(result[0].first) + ", " +
                      result[0].second + ".";
        }

        // COLLECT RESULTS FOR BUCKET MATCHING
        result = lev.query(non_exact_match_test, test_count);

        if (result.size() != std::min(test_count, words.size())) {
            passed = false;
            reason += "Expected " + std::to_string(std::min(test_count, words.size())) + "results but got " +
                      std::to_string(result.size()) + ".";
        }

        std::sort(result.begin(), result.end());

        // for (auto entry : result) std::cout << entry.first << " " << entry.second << std::endl;

        float last_penalty = (*(result.end() - 1)).first;

        for (int i = 0; std::abs(result[i].first - last_penalty) >= 1e-6; i++) {

            if (result[i].second != compare_result[i].second) {
                passed = false;
                reason += "Mismatch in result vectors: " + result[i].second + ", " + compare_result[i].second;
            }
        }

        if (passed)
            std::cout << "ok" << std::endl;
        else {
            std::cout << "FAIL: " << reason << std::endl;
            all_passed = false;
        }
    });

    return !all_passed;
}