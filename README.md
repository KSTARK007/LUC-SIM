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
./setup.sh
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

**NOTE: This simulator requires workload trace files to run. You must have trace files available before running the simulator.**

The traces we used are from the following datasets:
- Alibaba: https://github.com/alibaba/block-traces
- Meta: https://cachelib.org/docs/Cache_Library_User_Guides/Cachebench_FB_HW_eval/
- Twitter: https://github.com/twitter/cache-trace


The simulator **only** runs with JSON configuration files. You must configure `config.json` with the correct parameters:

**Required JSON Configuration Format:**
```json
{
  "num_threads": 1,
  "num_replicas": 3,
  "total_dataset_size": <dataset_size (int)>,
  "requests_per_thread": <requests_per_thread (int)>,
  "cache_percentage": <cache_percentage (0.01-1)>,
  "rdma_enabled": <true/false>,
  "enable_cba": <true/false>,
  "enable_de_duplication": <true/false>,
  "is_access_rate_fixed": <true/false>,
  "fixed_access_rate_value": <fixed_access_rate_value (int, default: 100000000)>,
  "cba_update_interval": <cba_update_interval (int, default: 100000000 [us])>,
  "latency_local": <latency_local (int, default: 1 [us][number taken from experiments in the paper using real system])>,
  "latency_rdma": <latency_rdma (int, default: 19 [us][number taken from experiments in the paper using real system])>,
  "latency_disk": <latency_disk (int, default: 296 [us][number taken from experiments in the paper using real system])>,
  "workload_folder": "/absolute/path/to/your/trace/files",
  "cache_type": <LRU/S3FIFO>
}
```

**Critical Requirements:**
- `workload_folder`: Must point to a directory containing `.txt` trace files
- `cache_type`: Must be either "LRU" or "S3FIFO"
- `total_dataset_size`: Should match your actual dataset size

### Step 4: Run the Simulation

```bash
./build/CacheSimulator config.json
```

**Other Scripts Available (Advanced):**
- `run_cache_simulator.sh`: Generates config and runs multiple scenarios
- `run_cdf_simulations.sh`: Runs CDF analysis experiments
- These scripts expect specific directory structures and may need modification for your setup

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

---

### Using Your Own Workload Traces

**Trace File Requirements:**
1. Place trace files in a directory
2. Files must have `.txt` extension  
3. Update `workload_folder` in config.json to point to this directory

**Directory Structure:**
```
/your/trace/directory/
├── workload_trace_1.txt
├── workload_trace_2.txt
└── workload_trace_N.txt
```

### Batch Simulations (Advanced)

For research experiments, you can modify the provided scripts:
```bash
# Note: These scripts expect /vectordb1/traces/ structure
# You may need to modify paths for your setup
./run_cache_simulator.sh <workload_number> <dataset_size> <cba_interval> <cache_policy> <workload_type>
```