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
df = pd.DataFrame(pd.read_csv("time_trie.txt", delim_whitespace=True))


## EXTRACT GENERAL INFORMATION

datasets = {}
iterations = df["#it"].max() + 1
threads = sorted(df["threads"].unique())
impls = sorted(df["impl"].unique())
tests = df["test"].unique()  # t_insert is insert

for dataset in df["dataset"].unique():
    datasets[dataset] = {
        "word_counts": sorted(df[df["dataset"] == dataset]["words"].unique())
    }

# Plot Time for Workload by Word Count
common_workloads = list(filter(lambda x: x != "t_insert", tests))
for dataset in datasets:

    fig, axs = plt.subplots(len(common_workloads))

    for i, workload in enumerate(common_workloads):

        for impl in impls:
            means = df
            means = means[means["dataset"] == dataset]
            means = means[np.logical_or(means["threads"] == threads[-1], np.logical_and(means["threads"] == 1, means["test"] == "t_count_children"))]
            means = means[means["key"] == "total"]
            means = means[means["test"] == workload]
            means = means[means["impl"] == impl]

            sns.lineplot(data=means, x="words", y="value", ax=axs[i], label=impl)

            axs[i].legend()
            axs[i].set_xlabel("Word Count")
            axs[i].set_ylabel("Mean of Time [ms], 95% CI")
            # axs[i].set_yscale("log")
            axs[i].set_title(f"{dataset}: Mean time for {workload}, {threads[-1]} threads, {iterations} iterations")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(axs) * DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"trie_workloads_word_count_{dataset}.pdf")


# Plot Time for Workload by Threads

# Only parallel workloads
common_workloads = list(filter(lambda x: x != "t_insert" and x != "t_count_children", tests))
for dataset in datasets:

    fig, axs = plt.subplots(len(common_workloads))

    for i, workload in enumerate(common_workloads):

        for impl in impls:
            means = df
            means = means[means["dataset"] == dataset]
            means = means[means["key"] == "total"]
            means = means[means["test"] == workload]
            means = means[means["impl"] == impl]
            means = means[means["words"] == datasets[dataset]["word_counts"][-1]]

            sns.lineplot(data=means, x="threads", y="value", ax=axs[i], label=impl)

            axs[i].legend()
            axs[i].set_xlabel("Threads")
            axs[i].set_ylabel("Mean of Time [ms], 95% CI")
            # axs[i].set_yscale("log")
            axs[i].set_title(f"{dataset}: Mean time for {workload}, {datasets[dataset]['word_counts'][-1]} words, {iterations} iterations")

    fig.set_size_inches(DEFAULT_FIG_SIZE[0], len(axs) * DEFAULT_FIG_SIZE[1])
    fig.tight_layout()
    fig.savefig(f"trie_workloads_threads_{dataset}.pdf")
