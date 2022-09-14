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
queries = df["query_idx"].max() + 1
max_n = df["n"].max()
threads = sorted(df["threads"].unique())
impls = sorted(df["impl"].unique())
query_types = df["query_type"].unique()

for dataset in df["dataset"].unique():
    datasets[dataset] = {
        "word_counts": sorted(df[df["dataset"] == dataset]["words"].unique())
    }

## COLLAPSE ITERATIONS

sequential_impls = ["naive_sequential", "accelerated_seq_vt", "accelerated_seq_vt_ne"]

# Collapses index, value, iterations
iterations_group = df.groupby(["dataset", "words", "query_idx", "query_type", "threads", "n", "impl", "type", "key"])
it_m = iterations_group.mean().reset_index()
it_s = iterations_group.std(ddof=0).reset_index()

## PLOT

for dataset in datasets:
    # Scatter Plot: time per query_size
    fig, ax = plt.subplots()

    means = it_m
    means = means[means["dataset"] == dataset]
    means = means[means["words"] == datasets[dataset]["word_counts"][-1]]
    means = means[means["type"] == "time"]
    means = means[means["key"] == "total"]
    means = means[means["query_type"] != "random"]

    for impl in impls:
        if impl in sequential_impls:
            impl_means = means[means["threads"] == 1]
        else:
            impl_means = means[means["threads"] == threads[-1]]

        impl_means = impl_means[impl_means["impl"] == impl]

        sns.lineplot(data=impl_means, x="n", y="value", ax=ax, label=impl)

    ax.legend()
    ax.set_xlabel("n: Number of Results")
    ax.set_ylabel("Mean of Time [ms], 95% CI")
    ax.set_yscale("log")
    ax.set_title(f"{dataset}: Mean time per Query, {threads[-1]} threads, {iterations} iterations, 50 queries (no random queries)")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"lev_query_time_by_n_{dataset}.pdf")
