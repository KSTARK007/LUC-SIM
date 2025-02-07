#!/bin/bash

CONFIG_FILE="config.json"
BASE_CONFIG='{
  "num_threads": 1,
  "num_replicas": 3,
  "total_dataset_size": 4467939,
  "requests_per_thread": 10000000,
  "cache_percentage": 0.33,
  "rdma_enabled": false,
  "enable_cba": false,
  "cba_update_interval": 10000000,
  "latency_local": 1,
  "latency_rdma": 15,
  "latency_disk": 200,
  "workload_folder": "/vectordb1/traces/twitter/7"
}'

# Function to update the JSON file and run the simulator in a screen session
run_simulator() {
    local rdma_enabled=$1
    local enable_cba=$2
    local session_name="rdma_${rdma_enabled}_cba_${enable_cba}"
    local output_log="results_${session_name}.log"

    echo "Starting simulation: $session_name in a screen session..."

    # Modify and save the configuration JSON file
    echo "$BASE_CONFIG" | jq --argjson rdma "$rdma_enabled" --argjson cba "$enable_cba" \
        '.rdma_enabled = $rdma | .enable_cba = $cba' > "$CONFIG_FILE"

    # Start a new screen session and run the simulator inside it
    screen -dmS "$session_name" bash -c "./build/CacheSimulator $CONFIG_FILE | tee $output_log"
}

# Ensure jq is installed
if ! command -v jq &> /dev/null; then
    echo "Error: jq is not installed. Please install jq to run this script."
    exit 1
fi

# Start all simulations in parallel using screen
run_simulator true true
sleep 10
run_simulator true false
sleep 10
run_simulator false false

echo "All simulations started in separate screen sessions."
echo "Use 'screen -ls' to list running sessions."
echo "To attach to a session, use 'screen -r session_name'."
echo "To detach, press 'Ctrl+A' then 'D'."
