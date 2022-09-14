import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import seaborn as sns
import numpy as np

sns.set_theme()
# mpl.style.use("ggplot")  # type: ignore
DEFAULT_FIG_SIZE = (10, 7)


## READ FILE
# Wrap in DataFrame to fix type hints
df = pd.DataFrame(pd.read_csv("time_levenshtein.txt", delim_whitespace=True))

datasets = {}
iterations = df["#it"].max() + 1
threads = sorted(df["threads"].unique())
impls = sorted(df["impl"].unique())
tests = df["test"].unique()  # t_insert is insert

for dataset in df["dataset"].unique():
    datasets[dataset] = {
        "word_counts": sorted(df[df["dataset"] == dataset]["words"].unique())
    }

df = df.groupby(["dataset", "threads", "impl", "test", "type", "key", "words"]).mean().reset_index()

for dataset in datasets:
    fig, ax = plt.subplots()

    index = ["accelerated_pt", "accelerated_pt_s"]
    index_names = ["Unsorted words (accelerated_pt)", "Sorted words (accelerated_pt_s)"]
    parts = {
        "lev: count children": {
            "data" : [],
            "keys" : ["total/precompute/compute_children", "total/precompute/compute_children"]
        },
        "trie: set indices": {
            "data" : [],
            "keys" : ["total/precompute/insert/set_indices", "total/precompute/insert/insert/set_indices"]
        },
        "trie: compress children": {
            "data" : [],
            "keys" : ["total/precompute/insert/compress", "total/precompute/insert/insert/compress"]
        },
        "trie: build trie": {
            "data" : [],
            "keys" : ["total/precompute/insert/build", "total/precompute/insert/insert/build"]
        },
        "trie: sort: copy": {
            "data" : [],
            "keys" : [0, "total/precompute/insert/copy"]
        },
        "trie: sort: parallel sort": {
            "data" : [],
            "keys" : [0, "total/precompute/insert/sort"]
        }
    }

    data = df
    data = data[data["dataset"] == dataset]
    data = data[data["words"] == datasets[dataset]["word_counts"][-1]]
    data = data[data["test"] == "l_precompute"]
    data = data[data["type"] == "time"]
    data = data[data["threads"] == threads[-1]]

    for part in parts:
        for i, key in enumerate(parts[part]["keys"]):
            impl_data = data[data["impl"] == index[i]]
            if key == 0:
                parts[part]["data"] += [0]
            else:
                value = impl_data[impl_data["key"] == key]["value"].to_numpy()
                parts[part]["data"] += [value[0]]
        parts[part] = parts[part]["data"]

    plot_df = pd.DataFrame(parts, index=index_names)
    plot_df.plot.bar(rot=0, ax=ax, stacked=True)
    ax.set_ylabel("Mean of time [ms]")
    ax.set_ylim(0, 200)
    ax.set_title(f"Precompute: unsorted vs. sorted words, {datasets[dataset]['word_counts'][-1]} words, {threads[-1]} threads, {iterations} iterations")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_precompute_parts_{dataset}.pdf")
