#!/bin/bash

# Define directories
TRACE_DIR="/vectordb1/traces/alibaba"
SIM_DIR="/vectordb1/LDC_Cache_Sim"
CODE_DIR="${TRACE_DIR}/code"
LOG_DIR="${TRACE_DIR}/logs"


# Create logs directory if it doesn't exist
mkdir -p "$LOG_DIR"

TRACES_NUMBER=(2020)
CachePolicy=("LRU" "S3FIFO")

for trace_number in "${TRACES_NUMBER[@]}"; do

    echo "Processing trace number ${trace_number}"

    # Define log file paths
    LOG_FILE="${LOG_DIR}/trace_${trace_number}.log"
    ERR_FILE="${LOG_DIR}/trace_${trace_number}.err"

    # Extract workload and dataset size
    STATS_FILE="./${trace_number}/stats.txt"
    if [[ ! -f "$STATS_FILE" ]]; then
        echo "Stats file not found for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

    # Extract values from stats.txt
    TOTAL_GET_REQUESTS=$(grep "Total 'get' requests" "$STATS_FILE" | awk '{print $NF}')
    DATASET_SIZE=$(grep "Unique keys" "$STATS_FILE" | awk '{print $NF}')
    CBA_UPDATE_INTERVAL=$(echo "$TOTAL_GET_REQUESTS * 0.05" | bc)

    # Validate extracted values
    if [[ -z "$DATASET_SIZE" || -z "$TOTAL_GET_REQUESTS" || -z "$CBA_UPDATE_INTERVAL" ]]; then
        echo "Failed to extract dataset size or update interval for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

    # Run cache simulator
    cd "$SIM_DIR" || { echo "Failed to navigate to ${SIM_DIR}" >>"$ERR_FILE"; continue; }
    for policy in "${CachePolicy[@]}"; do
        ./run_cache_simulator.sh "$trace_number" "$DATASET_SIZE" "$CBA_UPDATE_INTERVAL" "meta" >>"$LOG_FILE" 2>>"$ERR_FILE"
    done
    if [[ $? -ne 0 ]]; then
        echo "Cache simulator failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi
    cd "$TRACE_DIR" || { echo "Failed to navigate to ${TRACE_DIR}" >>"$ERR_FILE"; continue; }

    echo "Trace ${trace_number} completed successfully" >>"$LOG_FILE"

done

echo "All traces processed."
