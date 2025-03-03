import matplotlib.pyplot as plt
import numpy as np
import collections
import os
import argparse

def read_file(file_path):
    """Reads the file and extracts key-frequency pairs."""
    key_freq = collections.defaultdict(int)
    
    with open(file_path, 'r') as file:
        for line in file:
            parts = line.strip().split(',')
            if len(parts) == 2:
                key = parts[0].strip()
                freq = int(parts[1].strip())
                key_freq[key] += freq  # Aggregate frequencies for duplicate keys
    
    return key_freq

def plot_cdfs(key_freq, output_folder):
    """Plots and saves both regular and log-scale CDFs of access frequencies."""
    # Sort frequencies in descending order
    sorted_freqs = np.sort(list(key_freq.values()))[::-1]  # Reverse to get descending order
    total_accesses = np.sum(sorted_freqs)
    cumulative_freqs = np.cumsum(sorted_freqs)
    cdf = cumulative_freqs / total_accesses  # Normalize

    # Create figure with two subplots side by side
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    # Regular CDF
    # ax1.plot(range(len(sorted_freqs)), cdf, linestyle="-", color='blue')
    ax1.plot(range(len(sorted_freqs)), cdf, linestyle="-", color='blue')
    ax1.set_xlabel("Rank (Descending Order)")
    ax1.set_ylabel("Cumulative Probability")
    ax1.set_title("Cumulative Distribution Function (CDF)")
    ax1.grid(True)

    # Log-scale CDF
    ax2.plot(range(len(sorted_freqs)), cdf, linestyle="-", color='red')
    ax2.set_xscale('log')
    # ax2.set_yscale('log')
    ax2.set_xlabel("Rank (Descending Order, Log Scale)")
    ax2.set_ylabel("Cumulative Probability (Log Scale)")
    ax2.set_title("Log Cumulative Distribution Function")
    ax2.grid(True)

    # Add statistics annotation
    stats_text = f'Max Frequency: {sorted_freqs[0]:,}\n'
    stats_text += f'Min Frequency: {sorted_freqs[-1]:,}\n'
    stats_text += f'Mean Frequency: {int(np.mean(sorted_freqs)):,}'
    
    # Add text box with statistics
    plt.figtext(0.02, 0.02, stats_text, fontsize=8, 
                bbox=dict(facecolor='white', alpha=0.8))

    plt.tight_layout()
    output_path = os.path.join(output_folder, "cdf_plots.png")
    plt.savefig(output_path, bbox_inches='tight', dpi=300)
    plt.close()
    print(f"Saved CDF plots: {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Generate access pattern graphs")
    parser.add_argument("filename", type=str, help="File name with access patterns")
    parser.add_argument("folder", type=str, help="Folder containing the file")
    
    args = parser.parse_args()
    file_path = args.filename
    
    if not os.path.exists(file_path):
        print(f"Error: File {file_path} does not exist.")
        return
    
    key_freq = read_file(file_path)
    
    # Create output folder if it doesn't exist
    output_folder = os.path.join(args.folder, "output")
    os.makedirs(output_folder, exist_ok=True)
    
    plot_cdfs(key_freq, output_folder)

if __name__ == "__main__":
    main()