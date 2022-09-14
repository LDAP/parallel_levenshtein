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
accelerated_ds = ["accelerated_pt", "accelerated_vt"]
for dataset in datasets:
    # Scatter Plot: time per query_size
    fig, ax = plt.subplots(1)

    means = it_m
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    # means = means[means["type"] == "time"]
    means = means[means["n"] == 10]
    means = means[means["query_size"] == 10]
    # means = means[means["key"] == "total"]

    stds = it_s
    stds = stds[stds["dataset"] == dataset]
    stds = stds[stds["words"] == datasets[dataset]["word_counts"][-1]]
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
        sns.lineplot(data=impl_means, x="threads", y="value", label=impl)
        # ax.scatter(impl_means["query_size"], impl_means["value"], label=f"Time {impl}", alpha=0.5)


        ax.legend()
        ax.set_xlabel("Threads")
        ax.set_ylabel("Mean of Skipped Nodes, 95% CI")
        ax.set_title(f"{dataset}: Skipped Nodes, query_size = 10, n = 10, {iterations} iterations")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_skipped_nodes_by_threads_{dataset}.pdf")
