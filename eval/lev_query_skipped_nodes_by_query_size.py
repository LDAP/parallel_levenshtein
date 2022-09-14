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

## COLLAPSE ITERATIONS

# Collapses index, value, iterations
iterations_group = df.groupby(["dataset", "words", "threads", "n", "query_idx", "query_type","impl", "type", "key"])
it_m = iterations_group.mean().reset_index()
it_s = iterations_group.std(ddof=0).reset_index()

## PLOT
accelerated_ds = ["accelerated_pt", "accelerated_vt", "accelerated_pt_ne", "accelerated_vt_ne", "accelerated_seq_vt", "accelerated_seq_vt_ne"]
sequential_impls = ["accelerated_seq_vt", "accelerated_seq_vt_ne"]
for dataset in datasets:
    # Scatter Plot: time per query_size
    fig, ax = plt.subplots(1)
    ax2 = ax.twinx()

    means = it_m
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[np.logical_or(means["threads"] == threads[-1], np.logical_and(means["threads"] == 1, np.isin(means["impl"], sequential_impls)))]
    # means = means[means["type"] == "time"]
    means = means[means["n"] == 10]
    # means = means[means["key"] == "total"]

    stds = it_s
    stds = stds[stds["dataset"] == dataset]
    stds = stds[stds["words"] == datasets[dataset]["word_counts"][-1]]
    stds = stds[np.logical_or(stds["threads"] == threads[-1], np.logical_and(stds["threads"] == 1, np.isin(stds["impl"], sequential_impls)))]
    # stds = stds[stds["type"] == "time"]
    stds = stds[stds["n"] == 10]
    # stds = stds[stds["key"] == "total"]

    for i, impl in enumerate(accelerated_ds):
        impl_means = means[means["impl"] == impl]
        impl_stds = stds[stds["impl"] == impl]
        impl_means = impl_means[impl_means["key"] == "total"]
        impl_stds = impl_stds[impl_stds["key"] == "total"]

        # print(impl_means)

        # impl_means.plot.scatter("query_size", "value", yerr=impl_stds, ax=axs[i])
        #sns.regplot(x="query_size", y="value", data=impl_means, ax=ax, label=impl, lowess=True)
        ax.scatter(impl_means["query_size"], impl_means["value"], label=f"Time {impl}", alpha=0.5)


        ax.legend()
        ax.set_xlabel("Query Size")
        ax.set_ylabel("Mean of Time [ms] (⬤)")
        ax.set_yscale("log")
        ax.set_title(f"{dataset}: Time per Query, Skipped Nodes, {threads[-1]} threads, n = 10, {iterations} iterations")

        if impl in ["accelerated_pt", "accelerated_vt"]:
            impl_means = means[means["impl"] == impl]
            impl_stds = stds[stds["impl"] == impl]
            impl_means = impl_means[impl_means["key"] == "skipped_nodes"]
            impl_stds = impl_stds[impl_stds["key"] == "skipped_nodes"]

            ax2.scatter(impl_means["query_size"], impl_means["value"], marker='x', label=f"Skipped {impl}", alpha=0.5)
            ax2.legend()
            ax2.set_ylabel("Mean of Skipped Nodes (X)")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_skipped_nodes_by_query_size_no_err_{dataset}.pdf")

    ### Skipped nodes only with error

    accelerated_ds = ["accelerated_pt", "accelerated_vt"]

    fig, axs = plt.subplots(len(accelerated_ds))

    means = it_m
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[np.logical_or(means["threads"] == threads[-1], np.logical_and(means["threads"] == 1, np.isin(means["impl"], sequential_impls)))]
    # means = means[means["type"] == "time"]
    means = means[means["n"] == 10]
    # means = means[means["key"] == "total"]

    stds = it_s
    stds = stds[stds["dataset"] == dataset]
    stds = stds[stds["words"] == datasets[dataset]["word_counts"][-1]]
    stds = stds[np.logical_or(stds["threads"] == threads[-1], np.logical_and(stds["threads"] == 1, np.isin(stds["impl"], sequential_impls)))]
    # stds = stds[stds["type"] == "time"]
    stds = stds[stds["n"] == 10]
    # stds = stds[stds["key"] == "total"]

    for i, impl in enumerate(accelerated_ds):
        impl_means = means[means["impl"] == impl]
        impl_stds = stds[stds["impl"] == impl]
        impl_means = impl_means[impl_means["key"] == "skipped_nodes"]
        impl_stds = impl_stds[impl_stds["key"] == "skipped_nodes"]

        # impl_means.plot.scatter("query_size", "value", yerr=impl_stds, ax=axs[i])
        #sns.regplot(x="query_size", y="value", data=impl_means, ax=ax, label=impl, lowess=True)
        axs[i].errorbar(impl_means["query_size"], impl_means["value"], label=f"{impl}", alpha=0.5, yerr=impl_stds["value"], fmt='o', capsize=2)
        axs[i].set_xlabel("Query Size")
        axs[i].set_ylabel("Mean of Skipped Nodes, SD")
        axs[i].set_title(f"{dataset}: Number of skipped nodes, {impl}, {impl_means['threads'].max()} threads, n = 10, {iterations} iterations")
        axs[i].set_xscale('log')

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_skipped_nodes_by_query_size_with_err_{dataset}.pdf")
