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
df = pd.DataFrame(pd.read_csv("time_levenshtein_query.txt", delim_whitespace=True))


## EXTRACT GENERAL INFORMATION

datasets = {}
iterations = df["#it"].max() + 1
max_n = df["n"].max()
threads = sorted(df["threads"].unique())
impls = sorted(df["impl"].unique())
query_types = df["query_type"].unique()

for dataset in df["dataset"].unique():
    datasets[dataset] = {
        "word_counts": sorted(df[df["dataset"] == dataset]["words"].unique())
    }


fig, axs = plt.subplots(len(datasets))
for i, dataset in enumerate(datasets):
    means = df
    means = means[means["dataset"] == dataset]
    means = means[means["impl"] == "accelerated_vt"]
    means = means[means["threads"] == 16]
    means = means[means["n"] == 10]
    means = means[means["query_idx"] == 0]
    # means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[means["key"] == "num_nodes"]

    sns.lineplot(data=means, x="words", y="value", ax=axs[i])
    axs[i].set_xlabel("Word Count")
    axs[i].set_ylabel("Number of Nodes")
    axs[i].set_title(f"{dataset}: Number of Nodes")

fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(datasets) * DEFAULT_FIG_SIZE[1])
fig.tight_layout()
fig.savefig(f"trie_num_nodes_by_word_counts.pdf")
