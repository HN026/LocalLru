import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def plot_results(csv_file):
    if not os.path.exists(csv_file):
        print(f"CSV file {csv_file} not found.")
        sys.exit(1)

    # Load CSV
    df = pd.read_csv(csv_file)

    if not {"cache_type", "latency_us"}.issubset(df.columns):
        print("CSV missing required columns: cache_type, latency_us")
        sys.exit(1)

    # Group by cache type
    grouped = df.groupby("cache_type")

    # Plot latency distributions
    plt.figure(figsize=(10, 6))
    for name, group in grouped:
        plt.hist(group["latency_us"], bins=50, alpha=0.6, label=name)

    plt.xlabel("Latency (µs)")
    plt.ylabel("Frequency")
    plt.title("Cache Latency Distribution: LocalLRU vs LockCache")
    plt.legend()
    plt.grid(True)
    plt.show()

    # Boxplot comparison
    plt.figure(figsize=(8, 5))
    df.boxplot(column="latency_us", by="cache_type")
    plt.ylabel("Latency (µs)")
    plt.title("Latency Boxplot Comparison")
    plt.suptitle("")
    plt.grid(True)
    plt.show()

    # Print summary statistics
    print("\n=== Summary Statistics ===")
    print(df.groupby("cache_type")["latency_us"].describe())


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python scripts/plot_results.py <results.csv>")
        sys.exit(1)

    csv_file = sys.argv[1]
    plot_results(csv_file)
