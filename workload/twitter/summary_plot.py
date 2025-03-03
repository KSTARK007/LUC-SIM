import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Ensure output directory exists
output_dir = "plots"
os.makedirs(output_dir, exist_ok=True)

# Load the dataset (Modify the file path if needed)
csv_file = "summary.csv"  # Change this to your actual file
df = pd.read_csv(csv_file)

# Ensure proper column names
df.columns = df.columns.str.strip()

# Clean 'baseline diff' column (Remove % and convert to float)
df['baseline diff'] = df['baseline diff'].astype(str).str.replace('%', '', regex=False)
df['baseline diff'] = pd.to_numeric(df['baseline diff'], errors='coerce')  # Convert to float

# Drop any NaN values that could have resulted from conversion issues
df = df.dropna(subset=['baseline diff'])

# Compute summary statistics for baseline diff
summary_stats = df['baseline diff'].describe()
print("\nSummary Statistics for baseline diff (Random vs. CBA):")
print(summary_stats)

# Print additional statistics
print("\nAdditional Statistics for baseline diff:")
print(f"  Mean: {df['baseline diff'].mean():.2f}")
print(f"  Median: {df['baseline diff'].median():.2f}")
print(f"  Min: {df['baseline diff'].min():.2f}")
print(f"  Max: {df['baseline diff'].max():.2f}")
print(f"  Std Dev: {df['baseline diff'].std():.2f}")

## --- PLOTTING MULTIPLE VISUALIZATIONS ---

# 1. **Scatter Plot** - Performance Difference Across Clusters
plt.figure(figsize=(10, 5))
plt.scatter(df.index, df['baseline diff'], color='blue', alpha=0.7)
plt.axhline(y=100, color='red', linestyle='--', label="Mean baseline diff")
plt.xlabel("Cluster Index")
plt.ylabel("Baseline Diff (%)")
plt.title("Performance Difference Across Clusters (Random vs. CBA)")
plt.legend()
plt.grid(True)
plt.savefig(os.path.join(output_dir, "scatter_plot_baseline_diff.png"))
plt.close()

# 2. **Line Plot** - Performance Trend Across Clusters
plt.figure(figsize=(10, 5))
plt.plot(df.index, df['baseline diff'], marker='o', linestyle='-', color='purple', alpha=0.7)
plt.axhline(y=100, color='red', linestyle='--', label="Baseline performance")
plt.xlabel("Cluster Index")
plt.ylabel("Baseline Diff (%)")
plt.title("Trend of Performance Difference Across Clusters")
plt.legend()
plt.grid(True)
plt.savefig(os.path.join(output_dir, "line_plot_baseline_diff.png"))
plt.close()

# 3. **Histogram** - Distribution of Baseline Diff
plt.figure(figsize=(8, 5))
sns.histplot(df['baseline diff'], bins=50, kde=True, color='green', edgecolor='black')
plt.xlabel("Baseline Diff (%)")
plt.ylabel("Frequency")
plt.title("Histogram of Performance Differences")
plt.savefig(os.path.join(output_dir, "histogram_baseline_diff.png"))
plt.close()

print("\nGraphs have been saved in the 'plots/' directory.")
