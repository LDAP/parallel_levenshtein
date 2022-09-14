#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

class statistics_collector {

  private:
    statistics_collector() {
        reset();
    }
    statistics_collector(const statistics_collector&) = delete;
    statistics_collector& operator=(const statistics_collector&) = delete;
    statistics_collector(statistics_collector&&) = delete;
    statistics_collector& operator=(statistics_collector&&) = delete;

  public:
    void start_measure(std::string name = "") {
        parts.emplace_back(std::chrono::high_resolution_clock::now(), name, true);
        std::atomic_signal_fence(std::memory_order_seq_cst);
    }

    // calling multiple times is undefinied
    void stop_measure() {
        std::atomic_signal_fence(std::memory_order_seq_cst);
        parts.emplace_back(std::chrono::high_resolution_clock::now(), "", false);
    }

    std::vector<std::pair<std::string, double>> get_times(std::string delimiter = "/") {
        std::vector<std::pair<std::string, double>> result;
        std::vector<std::string> names;
        std::stack<std::chrono::high_resolution_clock::time_point> start_points;
        int validation = 0;

        for (auto part : parts) {
            if (std::get<2>(part)) {
                // start-point

                start_points.push(std::get<0>(part));
                names.push_back(std::get<1>(part));
                validation++;

            } else {
                // end-point
                std::chrono::high_resolution_clock::time_point end_point = std::get<0>(part);
                std::chrono::high_resolution_clock::time_point start_point = start_points.top();
                start_points.pop();

                double duration =
                    std::chrono::duration_cast<std::chrono::microseconds>(end_point - start_point).count() / 1000.;

                result.emplace_back(join(names, delimiter), duration);
                names.pop_back();
                validation--;
            }

            if (validation < 0) {
                throw std::runtime_error{"At least a measurement was not stopped!"};
            }
        }

        std::sort(result.begin(), result.end());
        return result;
    }

    void add_stat(std::string key, std::string value) {
        stats.emplace_back(std::move(key), std::move(value));
    }

    std::vector<std::pair<std::string, std::string>> get_stats() {
        return stats;
    }

    void reset() {
        parts.clear();
        parts.reserve(10);
        stats.clear();
        stats.reserve(10);
    }

  private:
    // time, name (if start), is_start
    std::vector<std::tuple<std::chrono::high_resolution_clock::time_point, std::string, bool>> parts;
    std::vector<std::pair<std::string, std::string>> stats;

    std::string join(std::vector<std::string> strings, std::string delim) {
        std::string result = strings[0];
        for (std::size_t i = 1; i < strings.size(); i++) {
            result += delim + strings[i];
        }
        return result;
    }

  public:
    static statistics_collector& get() {
        static statistics_collector instance;
        return instance;
    }
};