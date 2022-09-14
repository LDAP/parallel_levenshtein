#pragma once

#include <fstream>
#include <iostream>

namespace levenshtein {
class SimplePenalty {
    float modify(char from, char to) {
        if (from == to)
            return 0;
        if ((from ^ (1 << 5)) == to)
            return 0.1f;
        return 1.f;
    }

    float insert(char) {
        return 0.25f;
    }

    float remove(char) {
        return 1.f;
    }
};

class KBDistance {
  public:
    KBDistance(std::string path) {

        // First line: Letter probabilities
        // Then 26x26 lines "from to penalty"
        std::ifstream in(path);

        for (int i = 0; i < 26; i++) {
            in >> probabilities[i];
        }

        float max = 0;
        for (int i = 0; i < 26; i++) {
            for (int j = 0; j < 26; j++) {
                char f, t;
                float dist;
                in >> f >> t >> dist;

                max = std::max(max, dist);
                table[f - 'a'][t - 'a'] = dist;
            }
        }

        for (int i = 0; i < 26; i++) {
            for (int j = 0; j < 26; j++) {
                table[i][j] /= max;
            }
        }
    }

    float modify(char from, char to) const {
        if (from == to)
            return 0.f;

        from = from | (1 << 5);
        to = to | (1 << 5);

        if (from == to)
            return .02f;

        if (from <= 'z' && from >= 'a' && to <= 'z' && to >= 'a') {
            return .05f + table[from - 'a'][to - 'a'];
        }

        return 1.f;
    }

    float insert(char c) const {
        c = c | (1 << 5);

        if (c <= 'z' && c >= 'a') {
            return 0.7f - std::min(probabilities[c - 'a'] * 5, 0.65f);
        }

        return 1.f;
    }

    float remove(char c) const {
        c = c | (1 << 5);

        if (c <= 'z' && c >= 'a') {
            return 1 + probabilities[c - 'a'] * 5;
        }

        return 2.f;
    }

  private:
    float probabilities[26];
    float table[26][26];
};
} // namespace levenshtein
