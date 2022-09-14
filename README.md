# Parallel Levenshtein

This project provides a Trie-based parallel levenshtein implementation, intended to find the best fit from a large dictionary.

The demo below uses keyboard-distance and value frequency as penalty functions, suited to correct words from a mobile keyboard for example.

Runtime example: (using the German dictionary)

| Implementation         | Query                | Time (Speedup) |
|------------------------|----------------------|----------------|
| Naive Sequential       | Algorithmen          | 60 ms          |
| Naive Parallel-for[^1] | Algorithmen          | 11 ms (5.5x)   |
| Trie[^1]               | Algorithmen          | 0.95 ms (63x)  |

[^1]: AMD Ryzen 5800X, 8 Threads

Detailed experiments can be found in `results/`.

## Building

This project uses CMake. Build the project using:

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RELEASE ..
make
```

## Demo

```bash
cd build
# Supported 'en' and 'de'
./demo -lang en
```

**Example:**
```
Query: 
    alforuhm
Results:
    0.567 algorithm
    0.817 algorism
    0.867 aurorium
    0.952 algorithms
    0.978 alburnum
    1.089 allodium
    1.089 aphorism
    1.128 deltidium
    1.139 alsifilm
    1.139 ealdormen
```

## Experiments

To execute the experiments install the requirements from `requirements.txt` and run the `make_plots.py` python script.
