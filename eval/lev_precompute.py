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

sequential_impls = ["accelerated_st", "accelerated_st_s"]

df = df.groupby(["dataset", "threads", "impl", "test", "key", "words"]).mean().reset_index()

for dataset in datasets:
    fig, axs = plt.subplots(4)
    for impl in impls:

        if "accelerated" not in impl:
            continue

        ## TIME BY WORDS

        means = df
        means = means[means["dataset"] == dataset]
        # means = means[means["words"] == datasets["words"]["word_counts"][-1]]
        # if impl in sequential_impls:
        #     means = means[means["threads"] == 1]
        # else:
        #     means = means[means["threads"] == threads[-1]]
        means = means[means["threads"] == threads[-1]]
        means = means[means["impl"] == impl]
        means = means[means["test"] == "l_precompute"]
        means = means[means["key"] == "total/precompute"]

        sns.lineplot(data=means, x="words", y="value", ax=axs[0], label=impl)
        axs[0].set_title(f"{dataset}: Precompute, {threads[-1]} threads: Buildup time, {iterations} iterations")
        axs[0].set_xlabel("Number of words")
        axs[0].set_ylabel("Mean Buildup Time")

        ## TIME BY THREADS

        means = df
        means = means[means["dataset"] == dataset]
        means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
        # means = means[means["threads"] == threads[-1]]
        means = means[means["impl"] == impl]
        means = means[means["test"] == "l_precompute"]
        means = means[means["key"] == "total/precompute"]

        sns.lineplot(data=means, x="threads", y="value", ax=axs[1], label=impl)
        axs[1].set_title(f"{dataset}: Precompute, {datasets[dataset]['word_counts'][-1]} words: Buildup time, {iterations} iterations")
        axs[1].set_xlabel("Number of threads")
        axs[1].set_ylabel("Mean Buildup Time")
        # axs[1].set_yscale("log")

        ## SPEEDUP BY THREADS

        means = df
        means = means[means["dataset"] == dataset]
        means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
        # means = means[means["threads"] == threads[-1]]
        means = means[means["test"] == "l_precompute"]
        means = means[means["key"] == "total/precompute"]

        mean_sequential = means[means["impl"] == "accelerated_st"]["value"].mean()
        speedup = means[means["impl"] == impl].copy()

        speedup.loc[:, "value"] = mean_sequential / speedup["value"]

        sns.lineplot(data=speedup, x="threads", y="value", ax=axs[2], label=impl)
        axs[2].set_title(f"{dataset}: Precompute, {datasets[dataset]['word_counts'][-1]} words: Speedup, {iterations} iterations")
        axs[2].set_xlabel("Number of threads")
        axs[2].set_ylabel("Speedup")

        ## EFFICIENCY BY THREADS
        efficiency = speedup.copy()

        if impl not in sequential_impls:
            efficiency.loc[:, "value"] = efficiency.loc[:, "value"] / efficiency.loc[:, "threads"]


        sns.lineplot(data=efficiency, x="threads", y="value", ax=axs[3], label=impl)
        axs[3].set_title(f"{dataset}: Precompute, {datasets[dataset]['word_counts'][-1]} words: Efficiency, {iterations} iterations")
        axs[3].set_xlabel("Number of threads")
        axs[3].set_ylabel("Efficiency")


        fig.set_size_inches(DEFAULT_FIG_SIZE[0], 4 * DEFAULT_FIG_SIZE[1])
        fig.tight_layout()
        fig.savefig(f"lev_precompute_{dataset}.pdf")
