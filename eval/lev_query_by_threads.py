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

seq_impls = ["naive_sequential", "accelerated_seq_vt", "accelerated_seq_vt_ne"]

df = df[df["query_type"] != "random"]

df = df.groupby(["dataset", "words", "threads", "n", "impl", "type", "key"]).mean().reset_index()

for dataset in datasets:
    # Scatter Plot: time per query_size
    fig, axs = plt.subplots(3)

    ## TIME

    means = df
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[means["type"] == "time"]
    means = means[means["key"] == "total"]
    means = means[means["n"] == 10]

    # print(means[means["impl"] == "naive_sequential"])
    mean_sequential = means[means["impl"] == "accelerated_seq_vt"]["value"].mean()

    for impl in impls:
        impl_means = means[means["impl"] == impl].copy()

        if impl in seq_impls:
            copy = impl_means.copy()
            for thread in threads:
                if thread == 1:
                    continue
                copy["threads"] = thread
                means = means.append(copy, ignore_index=True)

        impl_means = means[means["impl"] == impl]

        sns.lineplot(data=impl_means, x="threads", y="value", ax=axs[0], label=impl)

    axs[0].legend()
    axs[0].set_xlabel("Number of Threads")
    axs[0].set_ylabel("Mean of Time [ms]")
    axs[0].set_yscale("log")
    axs[0].set_title(f"{dataset}: Mean time per Query, n = 10, {iterations} iterations, 50 queries (no random queries)")


    ## SPEEDUP

    for impl in impls:
        # if impl not in seq_impls:
        mask = means["impl"] == impl
        means.loc[mask, "value"] = mean_sequential / means[mask]["value"]

    for impl in impls:
        impl_means = means[means["impl"] == impl]

        sns.lineplot(data=impl_means, x="threads", y="value", ax=axs[1], label=impl)

    axs[1].legend()
    axs[1].set_xlabel("Number of Threads")
    axs[1].set_ylabel("Mean of Speedup")
    # axs[1].set_yscale("log")
    axs[1].set_title(f"{dataset}: Mean time per Query, n = 10, {iterations} iterations, 50 queries (no random queries)")

    ## EFFICIENCY

    for impl in impls:
        if impl not in seq_impls:
            mask = means["impl"] == impl
            means.loc[mask, "value"] = means[mask]["value"] / means[mask]["threads"]


    for impl in impls:
        impl_means = means[means["impl"] == impl]
        sns.lineplot(data=impl_means, x="threads", y="value", ax=axs[2], label=impl)

    axs[2].legend()
    axs[2].set_xlabel("Number of Threads")
    axs[2].set_ylabel("Mean of Efficiency")
    # axs[1].set_yscale("log")
    axs[2].set_title(f"{dataset}: Mean time per Query, n = 10, {iterations} iterations, 50 queries (no random queries)")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(axs) * DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_by_threads_{dataset}.pdf")
