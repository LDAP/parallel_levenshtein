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


implementations = [("accelerated_pt", "unsorted dictionary"), ("accelerated_pt_s", "sorted dictionary")]
for dataset in datasets:
    fig, axs = plt.subplots(2)
    for impl,name in implementations:

        ## BY WORDS

        means = df
        means = means[means["dataset"] == dataset]
        # means = means[means["words"] == datasets["words"]["word_counts"][-1]]
        means = means[means["threads"] == threads[-1]]
        means = means[means["impl"] == impl]
        means = means[means["type"] == "statistic"]
        means = means[means["key"] == "collisions"]

        sns.lineplot(data=means, x="words", y="value", ax=axs[0], label=name)
        axs[0].set_title(f"{dataset}: Trie Buildup, {threads[-1]} threads: Number of collisions, {iterations} iterations")
        axs[0].set_xlabel("Number of words")
        axs[0].set_ylabel("Mean number of collisions, 95% CI")

        ## BY THREADS

        means = df
        means = means[means["dataset"] == dataset]
        means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
        # means = means[means["threads"] == threads[-1]]
        means = means[means["impl"] == impl]
        means = means[means["type"] == "statistic"]
        means = means[means["key"] == "collisions"]

        sns.lineplot(data=means, x="threads", y="value", ax=axs[1], label=name)
        axs[1].set_title(f"{dataset}: Trie Buildup, {datasets[dataset]['word_counts'][-1]} words: Number of collisions, {iterations} iterations")
        axs[1].set_xlabel("Number of threads")
        axs[1].set_ylabel("Mean number of collisions, 95% CI")

        fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(datasets) * DEFAULT_FIG_SIZE[1])
        fig.tight_layout()
        fig.savefig(f"trie_buildup_collisions_{dataset}.pdf")
