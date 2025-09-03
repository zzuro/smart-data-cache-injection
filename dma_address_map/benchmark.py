import os
import subprocess
import re
import statistics
import csv
from tqdm import tqdm
import matplotlib.pyplot as plt

CSV_FILE = "benchmark_results.csv"
IMG_FILE = "benchmark_bar_diagram.png"

cores = [2, 4, 6, 8, 10]
args = ["", "no_tag", "no_th"]
ITER = 100

labels = {
    "": "With tag and TH",
    "no_tag": "Without tag",
    "no_th": "Without TH"
}

cycles = {arg: [] for arg in args}


def run_benchmark():
    """Run the benchmark and collect CPU cycles with progress bar."""
    total = len(cores) * len(args) * ITER
    with tqdm(total=total, desc="Benchmarking") as pbar:
        for core in cores:
            for arg in args:
                for _ in range(ITER):
                    cmd = ["sudo", "taskset", "-c", str(core), "./um"]
                    if arg:
                        cmd.append(arg)
                    result = subprocess.run(cmd, capture_output=True, text=True)
                    match = re.search(r"Access time:\s*(\d+)\s*CPU cycles", result.stdout)
                    if match:
                        cycle_val = int(match.group(1))
                        cycles[arg].append((core, cycle_val))
                    pbar.update(1)

    # Write results to CSV
    with open(CSV_FILE, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Argument", "Core", "CPU cycles"])
        for arg in args:
            for core, cycle_val in cycles[arg]:
                writer.writerow([arg, core, cycle_val])

def load_from_csv():
    """Load benchmark data from CSV file."""
    with open(CSV_FILE, newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            arg = row["Argument"]
            core = int(row["Core"])
            cycle_val = int(row["CPU cycles"])
            if arg in cycles:
                cycles[arg].append((core, cycle_val))


def plot_results():
    """Plot median CPU cycles per core and argument."""
    bar_width = 0.2
    x = range(len(cores))
    fig, ax = plt.subplots()

    for idx, arg in enumerate(args):
        medians = []
        for core in cores:
            core_cycles = [val for c, val in cycles[arg] if c == core]
            if core_cycles:
                median_val = statistics.median(core_cycles)
            else:
                median_val = 0
            medians.append(median_val)
        ax.bar([i + idx * bar_width for i in x], medians, width=bar_width, label=labels[arg])

    ax.set_xlabel("Core")
    ax.set_ylabel("CPU cycles")
    ax.set_title("Cache Access Time")
    ax.set_xticks([i + bar_width for i in x])
    ax.set_xticklabels([str(core) for core in cores])
    ax.legend()

    plt.tight_layout()
    plt.savefig(IMG_FILE)
    plt.close()


if __name__ == "__main__":
    if os.path.exists(CSV_FILE):
        print(f"CSV file '{CSV_FILE}' found. Loading results...")
        load_from_csv()
    else:
        print("No CSV file found. Running benchmark...")
        run_benchmark()

    print("Plotting results...")
    plot_results()
    print(f"Results saved to {IMG_FILE}")
