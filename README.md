# LDC Cache Simulator

A high-performance distributed cache simulation system for evaluating caching strategies, RDMA configurations, and cost-benefit analysis techniques using real-world workload traces.

## 🏗️ Architecture Overview

The LDC Cache Simulator models a **distributed caching system** with the following key components:

```
┌─────────────────────────────────────────────────────────┐
│                    Cache Simulator                      │
├─────────────────────────────────────────────────────────┤
│  RequestProcessor  │  Reads workload traces & sends     │
│                    │  requests to replica manager       │
├────────────────────┼─────────────────────────────────────┤
│  ReplicaManager    │  Routes requests to cache replicas │
│                    │  Handles RDMA simulation & CBA     │
├────────────────────┼─────────────────────────────────────┤
│  Cache Replicas    │  Individual cache nodes with:      │
│  (3 nodes)         │  • LRU or S3FIFO policies          │
│                    │  • Local/remote data fetching      │
│                    │  • Configurable capacities         │
├────────────────────┼─────────────────────────────────────┤
│  Metrics Collector │  Tracks performance metrics &      │
│                    │  writes results to files           │
└─────────────────────────────────────────────────────────┘
```

**Key Features:**
- **Multiple Cache Policies**: LRU (Least Recently Used) and S3FIFO (advanced FIFO-based)
- **RDMA Simulation**: Models remote direct memory access with configurable latencies
- **Cost-Benefit Analysis**: Optimizes cache placement based on access patterns
- **Real Workloads**: Uses traces from Alibaba, Meta, and Twitter production systems
- **Configurable Parameters**: Cache size, RDMA settings, deduplication, and more

## 🚀 How to Run

### Step 1: Install Dependencies

```bash
# Quick setup (recommended)
./setup.sh

# Or install manually
sudo apt update
sudo apt install cmake nlohmann-json3-dev libtbb-dev screen btop jq -y
```

### Step 2: Build the Project

```bash
# Using the build script (recommended)
./compile.sh

# Or build manually
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Step 3: Configure Your Simulation

The simulator uses JSON configuration files. You can:

**Option A: Use the default configuration**
```bash
# Uses config.json with predefined settings
./build/CacheSimulator config.json
```

**Option B: Generate custom configuration**
```bash
# Syntax: ./run_cache_simulator.sh <workload_number> <dataset_size> <cba_interval> <cache_policy> <workload_type>
./run_cache_simulator.sh 1 199757312 59999729.7 LRU alibaba
```

**Option C: Create your own config.json**
```json
{
  "num_threads": 1,
  "num_replicas": 3,
  "total_dataset_size": 199757312,
  "cache_percentage": 0.34,
  "rdma_enabled": true,
  "enable_cba": true,
  "cache_type": "LRU",
  "workload_folder": "/path/to/your/traces",
  "latency_local": 1,
  "latency_rdma": 19,
  "latency_disk": 296
}
```

### Step 4: Run Different Simulation Scenarios

```bash
# Single simulation
./build/CacheSimulator config.json

# Multiple configurations automatically
./run_cache_simulator.sh 2020 328491867 75500000 S3FIFO alibaba

# CDF analysis for research
./run_cdf_simulations.sh "1 2 3" 4467939 851370550
```

### Step 5: Monitor Progress

```bash
# View running simulations
screen -ls

# Attach to a specific simulation
screen -r simulation_session_name

# Check logs
tail -f results_*.log
```

## 📊 Results and Analysis

### Where Results Are Stored

All simulation results are saved in the `workload/` directory with a structured naming convention:

```
workload/
├── alibaba/2020/
├── meta/202206/
├── twitter/1/
│   ├── workload_twitter_1_cache_LRU_34_rdma_cba_access_ratevariable_no_dedup.txt
│   ├── workload_twitter_1_cache_LRU_34_no_rdma_no_cba_access_ratevariable_no_dedup.txt
│   └── workload_twitter_1_cache_S3FIFO_34_rdma_cba_access_ratevariable_no_dedup.txt
└── twitter/plots/
    ├── summary.csv
    ├── scatter_plot_baseline_diff.png
    └── summary_statistics_baseline_diff.csv
```

### Understanding the Naming Convention

Result files follow this pattern:
```
workload_{dataset}_{number}_cache_{policy}_{cache_pct}_{rdma}_{cba}_access_rate{fixed/variable}_{dedup}.txt
```

**Example**: `workload_twitter_1_cache_LRU_34_rdma_cba_access_ratevariable_no_dedup.txt`
- `twitter_1`: Twitter dataset #1
- `LRU`: Cache policy used
- `34`: 34% cache size relative to dataset
- `rdma`: RDMA enabled
- `cba`: Cost-benefit analysis enabled
- `variable`: Variable access rate
- `no_dedup`: Deduplication disabled

### Key Metrics in Results

Each result file contains:

```
=== Performance Metrics ===
Total Requests: 1000000
Cache Hits: 340000
Cache Misses: 660000
Hit Rate: 34.0%
Remote Fetches: 450000
Average Latency: 89.5ms
RDMA Usage: 78.2%
CBA Optimizations: 1240
```

### Visualizing Results

```bash
# Generate plots for Twitter workloads
cd workload/twitter/
python3 summary_plot.py

# View generated visualizations
ls plots/
# Files: histogram_baseline_diff.png, line_plot_baseline_diff.png, scatter_plot_baseline_diff.png
```

### Comparing Configurations

To compare different configurations:

1. **Find your result files** in `workload/{dataset}/{number}/`
2. **Compare hit rates** between different cache policies
3. **Analyze RDMA impact** by comparing `rdma` vs `no_rdma` files
4. **Evaluate CBA effectiveness** by comparing `cba` vs `no_cba` files

### Example Analysis

```bash
# Compare LRU vs S3FIFO for Twitter dataset 1
grep "Hit Rate" workload/twitter/1/workload_twitter_1_cache_LRU_34_*
grep "Hit Rate" workload/twitter/1/workload_twitter_1_cache_S3FIFO_34_*

# Check RDMA effectiveness
grep "Remote Fetches" workload/twitter/1/*rdma*
grep "Remote Fetches" workload/twitter/1/*no_rdma*
```
---

### Batch Simulations

For research experiments, use the provided scripts:
```bash
# Test multiple configurations
for policy in LRU S3FIFO; do
  for workload in 1 2 3; do
    ./run_cache_simulator.sh $workload 199757312 59999729.7 $policy twitter
  done
done
```