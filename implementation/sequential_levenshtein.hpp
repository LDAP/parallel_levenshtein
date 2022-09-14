#pragma once

#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "implementation/levenshtein_penalty_functions.hpp"

template <class PenaltyClass = levenshtein::KBDistance> class SequentialLevenshtein {
  public:
    SequentialLevenshtein(PenaltyClass penalty) : penalty(penalty) {}

    void precompute(std::vector<std::string>& words) noexcept {
        _words = words;
    }

    std::vector<std::pair<float, std::string>> query(const std::string& query, std::size_t n) noexcept {

        std::priority_queue<std::pair<float, std::size_t>> q;

        for (std::size_t i = 0; i < _words.size(); i++) {
            std::string& word = _words[i];
            float dist = edit_distance(word, query);
            if (q.size() < n || dist < q.top().first) { // order matters here
                q.emplace(dist, i);
                if (q.size() > n)
                    q.pop();
            }
        }

        std::vector<std::pair<float, std::string>> result(q.size());

        while (!q.empty()) {
            result[q.size() - 1] = std::make_pair(q.top().first, _words[q.top().second]);
            q.pop();
        }

        return result;
    }

    inline float edit_distance(const std::string& word, const std::string& query) noexcept {
        const std::size_t m = word.size();
        const std::size_t n = query.size();

        float dp[m + 1][n + 1];

        // Prepare
        dp[0][0] = 0.f;

        for (std::size_t i = 1; i <= m; i++) {
            dp[i][0] = dp[i - 1][0] + penalty.insert(word[i - 1]);
        }

        for (std::size_t i = 1; i <= n; i++) {
            dp[0][i] = dp[0][i - 1] + penalty.remove(query[i - 1]);
        }

        // Calculate DP
        for (std::size_t i = 1; i <= m; i++) {
            for (std::size_t j = 1; j <= n; j++) {
                dp[i][j] = std::min(
                    std::min(dp[i - 1][j] + penalty.insert(word[i - 1]), dp[i][j - 1] + penalty.remove(query[j - 1])),
                    dp[i - 1][j - 1] + penalty.modify(word[i - 1], query[j - 1]));
            }
        }

        return dp[m][n];
    }

  private:
    PenaltyClass penalty;
    std::vector<std::string> _words;
};
