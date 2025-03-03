#!/bin/bash
# WORKLOAD_NUMBER=19
# DATASET_SIZE=328491867
# CBA_UPDATE_INTERVAL=75500000
WORKLOAD_NUMBER=$1
DATASET_SIZE=$2
CBA_UPDATE_INTERVAL=$3

CONFIG_FILE="config.json"

generate_config() {
    jq -n \
        --argjson num_threads 1 \
        --argjson num_replicas 3 \
        --argjson total_dataset_size "$DATASET_SIZE" \
        --argjson requests_per_thread 10000000 \
        --argjson cache_percentage 0.34 \
        --argjson rdma_enabled "$1" \
        --argjson enable_cba "$2" \
        --argjson enable_de_duplication false \
        --argjson is_access_rate_fixed false \
        --argjson fixed_access_rate_value 1000000 \
        --argjson cba_update_interval "$CBA_UPDATE_INTERVAL" \
        --argjson latency_local 1 \
        --argjson latency_rdma 19 \
        --argjson latency_disk 296 \
        --arg workload_folder "/vectordb1/traces/twitter/$WORKLOAD_NUMBER" \
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
            workload_folder: $workload_folder
        }' > "$CONFIG_FILE"
}

# Function to update the JSON file and run the simulator in a screen session
run_simulator() {
    local rdma_enabled=$1
    local enable_cba=$2
    local session_name="${WORKLOAD_NUMBER}_rdma_${rdma_enabled}_cba_${enable_cba}"
    local output_log="results_${session_name}.log"

    echo "Starting simulation: $session_name in a screen session..."

    # Modify and save the configuration JSON file
    generate_config "$rdma_enabled" "$enable_cba"

    # Start a new screen session and run the simulator inside it
    # screen -dmS "$session_name" bash -c "./build/CacheSimulator $CONFIG_FILE | tee $output_log"
    ./build/CacheSimulator $CONFIG_FILE | tee $output_log
}

# Ensure jq is installed
if ! command -v jq &> /dev/null; then
    echo "Error: jq is not installed. Please install jq to run this script."
    exit 1
fi

run_simulator true true
run_simulator true false
run_simulator false false