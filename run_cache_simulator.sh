#!/bin/bash
WORKLOAD_NUMBER=19
DATASET_SIZE=328491867
CBA_UPDATE_INTERVAL=75500000

CONFIG_FILE="config.json"
BASE_CONFIG='{
  "num_threads": 1,
  "num_replicas": 3,
  "total_dataset_size": '$DATASET_SIZE',
  "requests_per_thread": 10000000,
  "cache_percentage": 0.34,
  "rdma_enabled": false,
  "enable_cba": false,
  "enable_de_duplication": false,
  "is_access_rate_fixed": false,
  "fixed_access_rate_value": 1000000,
  "cba_update_interval": '$CBA_UPDATE_INTERVAL',
  "latency_local": 1,
  "latency_rdma": 19,
  "latency_disk": 296,
  "workload_folder": "/vectordb1/traces/twitter/'$WORKLOAD_NUMBER'"
}'

# Function to update the JSON file and run the simulator in a screen session
run_simulator() {
    local rdma_enabled=$1
    local enable_cba=$2
    local session_name="${WORKLOAD_NUMBER}_rdma_${rdma_enabled}_cba_${enable_cba}"
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
# sleep 10
# run_simulator true false
# sleep 10
# run_simulator false false

echo "All simulations started in separate screen sessions."
echo "Use 'screen -ls' to list running sessions."
echo "To attach to a session, use 'screen -r session_name'."
echo "To detach, press 'Ctrl+A' then 'D'."
