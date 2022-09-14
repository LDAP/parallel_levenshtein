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

## COLLAPSE ITERATIONS

# Collapses index, value, iterations
iterations_group = df.groupby(["dataset", "words", "threads", "n", "query_idx", "query_type","impl", "type", "key"])
it_m = iterations_group.mean().reset_index()
it_s = iterations_group.std(ddof=0).reset_index()

## PLOT

for dataset in datasets:
    # Scatter Plot: time per query_size
    fig, axs = plt.subplots(len(impls))

    means = it_m
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[np.logical_or(means["threads"] == threads[-1], np.logical_and(means["threads"] == 1, np.isin(means["impl"], seq_impls)))]
    means = means[means["type"] == "time"]
    means = means[means["key"] == "total"]
    means = means[means["n"] == 10]

    stds = it_s
    stds = stds[stds["dataset"] == dataset]
    stds = stds[stds["words"] == datasets[dataset]["word_counts"][-1]]
    stds = stds[np.logical_or(stds["threads"] == threads[-1], np.logical_and(stds["threads"] == 1, np.isin(stds["impl"], seq_impls)))]
    stds = stds[stds["type"] == "time"]
    stds = stds[stds["key"] == "total"]
    stds = stds[stds["n"] == 10]

    for i, impl in enumerate(impls):
        impl_means = means[means["impl"] == impl]
        impl_stds = stds[stds["impl"] == impl]

        # impl_means.plot.scatter("query_size", "value", yerr=impl_stds, ax=axs[i])
        #sns.regplot(x="query_size", y="value", data=impl_means, ax=ax, label=impl, lowess=True)
        for query_type in query_types:
            type_means = impl_means[impl_means["query_type"] == query_type]
            type_stds = impl_stds[impl_stds["query_type"] == query_type]
            axs[i].errorbar(type_means["query_size"], type_means["value"], yerr=type_stds["value"], fmt='o', capsize=2, label=query_type, alpha=0.5)


        axs[i].legend()
        axs[i].set_xlabel("Query Size")
        axs[i].set_ylabel("Mean of Time [ms], SD")
        axs[i].set_title(f"{dataset}: Time per Query, {impl}, {impl_means['threads'].max()} threads, n = 10, {iterations} iterations")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(axs) * DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_time_by_query_size_{dataset}.pdf")

    # Scatter Plot: time per query_size all in one
    fig, ax = plt.subplots(1)
    for i, impl in enumerate(impls):
        impl_means = means[means["impl"] == impl]
        # impl_means = impl_means[impl_means["query_type"] == "random"]

        sns.regplot(x="query_size", y="value", data=impl_means, ax=ax, label=f"{impl}, {impl_means['threads'].max()} threads", lowess=True)


    ax.legend()
    ax.set_xlabel("Query Size")
    ax.set_ylabel("Mean of Time [ms]")
    ax.set_title(f"{dataset}: Time per Query, n = 10, {iterations} iterations")
    ax.set_yscale('log')

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_time_by_query_size_{dataset}_single_no_err.pdf")
