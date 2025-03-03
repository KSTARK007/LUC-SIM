#!/bin/bash

# Define required variables (ensure these are set before running)
DATASET_SIZE=328491867  # Example dataset size
CBA_UPDATE_INTERVAL=75500000  # Example interval
WORKLOAD="meta"
WORKLOAD_NUMBER="19"
CACHE_POLICY="S3FIFO"
CONFIG_FILE="test.json"

# Generate JSON using jq
jq -n \
    --argjson num_threads 1 \
    --argjson num_replicas 3 \
    --argjson total_dataset_size "$DATASET_SIZE" \
    --argjson requests_per_thread 10000000 \
    --argjson cache_percentage 0.34 \
    --argjson rdma_enabled 1 \
    --argjson enable_cba 2 \
    --argjson enable_de_duplication false \
    --argjson is_access_rate_fixed false \
    --argjson fixed_access_rate_value 1000000 \
    --argjson cba_update_interval "$CBA_UPDATE_INTERVAL" \
    --argjson latency_local 1 \
    --argjson latency_rdma 19 \
    --argjson latency_disk 296 \
    --arg workload_folder "/vectordb1/traces/$WORKLOAD/$WORKLOAD_NUMBER" \
    --arg cache_type "$CACHE_POLICY" \
    '{
        num_threads: $num_threads,
        num_replicas: $num_replicas,
        total_dataset_size: $total_dataset_size,
        requests_per_thread: $requests_per_thread,
        cache_percentage: $cache_percentage,
        rdma_enabled: $rdma_enabled,
        enable_cba: $enable_cba,
        enable_de_duplication: $enable_de_duplication,
        is_access_rate_fixed: $is_access_rate_fixed,
        fixed_access_rate_value: $fixed_access_rate_value,
        cba_update_interval: $cba_update_interval,
        latency_local: $latency_local,
        latency_rdma: $latency_rdma,
        latency_disk: $latency_disk,
        workload_folder: $workload_folder,
        cache_type: $cache_type
    }' > "$CONFIG_FILE"

echo "Configuration file $CONFIG_FILE generated successfully."
