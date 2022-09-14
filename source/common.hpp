#pragma once

#include "implementation/accelerated_levenshtein.hpp"
#include "implementation/accelerated_levenshtein_sequential.hpp"
#include "implementation/naive_levenshtein.hpp"
#include "implementation/sequential_levenshtein.hpp"
#include "implementation/sequential_trie.hpp"
#include "implementation/sorted_buildup_trie.hpp"
#include "implementation/trie.hpp"
#include "implementation/vectorized_trie.hpp"

#include <string>

// NOTATION
// SEQ: optimized Sequential implementation, PAR: Parallel implementation
// SORTED: Additional sorting step
// VT: Vectorized Trie, ST: Sequential Trie, PT: Parallel Trie
// S: SortedBuildup Trie
// NE: No early break

// ---------- TRIE IMPLEMENTATIONS --------------

struct TRIE_SEQ {
    bool seq = true;
    std::string name = "sequential";
    static auto make([[maybe_unused]] std::size_t num_threads) {
        return SequentialTrie<DummyPayload>();
    }
};

struct TRIE_PAR {
    bool seq = false;
    std::string name = "parallel";
    static auto make(std::size_t num_threads) {
        return ParallelTrie<DummyPayload, true>(num_threads);
    }
};

struct TRIE_PAR_VECTORIZED {
    bool seq = false;
    std::string name = "parallel_vectorized";
    static auto make(std::size_t num_threads) {
        return VectorizedParallelTrie<DummyPayload, true>(num_threads);
    }
};

struct TRIE_SEQ_SORTED {
    bool seq = true;
    std::string name = "sorted_buildup_sequential";
    static auto make(std::size_t num_threads) {
        return SortedBuildupTrie<DummyPayload, SequentialTrie<DummyPayload>, true>(num_threads);
    }
};

struct TRIE_PAR_SORTED {
    bool seq = false;
    std::string name = "sorted_buildup_parallel";
    static auto make(std::size_t num_threads) {
        return SortedBuildupTrie<DummyPayload, ParallelTrie<DummyPayload, true>, true>(num_threads);
    }
};

struct TRIE_PAR_VECTORIZED_SORTED {
    bool seq = false;
    std::string name = "sorted_buildup_parallel_vectorized";
    static auto make(std::size_t num_threads) {
        return SortedBuildupTrie<DummyPayload, VectorizedParallelTrie<DummyPayload, true>, true>(num_threads);
    }
};

const auto trie_impls = std::make_tuple(TRIE_SEQ(), TRIE_PAR(), TRIE_PAR_VECTORIZED(), TRIE_SEQ_SORTED(),
                                        TRIE_PAR_SORTED(), TRIE_PAR_VECTORIZED_SORTED());

// ---------- LEVENSHTEIN IMPLEMENTATIONS --------------

struct LEV_NAIVE_SEQ {
    bool seq = true;
    std::string name = "naive_sequential";

    template <class PenaltyClass> static auto make([[maybe_unused]] std::size_t num_threads, PenaltyClass penalty) {
        return SequentialLevenshtein<PenaltyClass>(penalty);
    }
};

struct LEV_NAIVE_PAR {
    bool seq = false;
    std::string name = "naive_parallel";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return NaiveLevenshtein<PenaltyClass>(penalty, num_threads);
    }
};

struct LEV_ACCELERATED_SEQ {
    bool seq = true;
    std::string name = "accelerated_seq_vt";

    template <class PenaltyClass> static auto make([[maybe_unused]] std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshteinSeq<PenaltyClass, VectorizedParallelTrie<SeqLevTriePayload, true>, true, true>(
            penalty);
    }
};

struct LEV_ACCELERATED_SEQ_NE {
    bool seq = true;
    std::string name = "accelerated_seq_vt_ne";

    template <class PenaltyClass> static auto make([[maybe_unused]] std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshteinSeq<PenaltyClass, VectorizedParallelTrie<SeqLevTriePayload, true>, false, true>(
            penalty);
    }
};

struct LEV_ACCELERATED_VT {
    bool seq = false;
    std::string name = "accelerated_vt";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, VectorizedParallelTrie<TriePayload, true>, true, true>(penalty,
                                                                                                           num_threads);
    }
};

struct LEV_ACCELERATED_VT_NE {
    bool seq = false;
    std::string name = "accelerated_vt_ne";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, VectorizedParallelTrie<TriePayload, true>, false, true>(
            penalty, num_threads);
    }
};

struct LEV_ACCELERATED_ST {
    bool seq = false;
    std::string name = "accelerated_st";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, SequentialTrie<TriePayload>, true, true>(penalty, num_threads);
    }
};

struct LEV_ACCELERATED_PT {
    bool seq = false;
    std::string name = "accelerated_pt";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, ParallelTrie<TriePayload, true>, true, true>(penalty, num_threads);
    }
};

struct LEV_ACCELERATED_PT_NE {
    bool seq = false;
    std::string name = "accelerated_pt_ne";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, ParallelTrie<TriePayload, true>, false, true>(penalty, num_threads);
    }
};

struct LEV_ACCELERATED_VT_S {
    bool seq = false;
    std::string name = "accelerated_vt_s";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<
            PenaltyClass, SortedBuildupTrie<TriePayload, VectorizedParallelTrie<TriePayload, true>, true>, true, true>(
            penalty, num_threads);
    }
};

struct LEV_ACCELERATED_ST_S {
    bool seq = false;
    std::string name = "accelerated_st_s";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<PenaltyClass, SortedBuildupTrie<TriePayload, SequentialTrie<TriePayload>, true>,
                                      true, true>(penalty, num_threads);
    }
};

struct LEV_ACCELERATED_PT_S {
    bool seq = false;
    std::string name = "accelerated_pt_s";

    template <class PenaltyClass> static auto make(std::size_t num_threads, PenaltyClass penalty) {
        return AcceleratedLevenshtein<
            PenaltyClass, SortedBuildupTrie<TriePayload, ParallelTrie<TriePayload, true>, true>, true, true>(
            penalty, num_threads);
    }
};

const auto all_levenshtein_impls =
    std::make_tuple(LEV_NAIVE_SEQ(), LEV_NAIVE_PAR(), LEV_ACCELERATED_SEQ(), LEV_ACCELERATED_VT(), LEV_ACCELERATED_ST(),
                    LEV_ACCELERATED_PT(), LEV_ACCELERATED_VT_S(), LEV_ACCELERATED_ST_S(), LEV_ACCELERATED_PT_S(),
                    LEV_ACCELERATED_SEQ_NE(), // no early break
                    LEV_ACCELERATED_PT_NE(), LEV_ACCELERATED_VT_NE());

const auto precompute_levenshtein_impls =
    std::make_tuple(LEV_NAIVE_SEQ(), LEV_NAIVE_PAR(), LEV_ACCELERATED_SEQ(), LEV_ACCELERATED_VT(), LEV_ACCELERATED_ST(),
                    LEV_ACCELERATED_PT(), LEV_ACCELERATED_VT_S(), LEV_ACCELERATED_ST_S(), LEV_ACCELERATED_PT_S());

const auto query_levenshtein_impls =
    std::make_tuple(LEV_NAIVE_SEQ(), LEV_NAIVE_PAR(), LEV_ACCELERATED_SEQ(), LEV_ACCELERATED_PT(), LEV_ACCELERATED_VT(),
                    LEV_ACCELERATED_SEQ_NE(), // no early break
                    LEV_ACCELERATED_PT_NE(), LEV_ACCELERATED_VT_NE());

// ---------- HELPER ------------

// https://stackoverflow.com/questions/26902633/how-to-iterate-over-a-stdtuple-in-c-11
template <class F, class... Ts, std::size_t... Is>
void for_each_in_tuple(const std::tuple<Ts...>& tuple, F func, std::index_sequence<Is...>) {
    using expander = int[];
    (void)expander{0, ((void)func(std::get<Is>(tuple)), 0)...};
}

template <class F, class... Ts> void for_each_in_tuple(const std::tuple<Ts...>& tuple, F func) {
    for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}
