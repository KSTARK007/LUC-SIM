#!/bin/bash

# Define directories
TRACE_DIR="/mydata/traces/twitter"
SIM_DIR="/mydata/LDC_Cache_Sim"
CODE_DIR="${TRACE_DIR}/code"
LOG_DIR="${TRACE_DIR}/logs"

# Create logs directory if it doesn't exist
mkdir -p "$LOG_DIR"

# Define trace numbers to skip
SKIP_TRACES=(4 7 8 12 14 15 19 21 22 26 32 38 39 47 50)

# Iterate through trace numbers 1 to 54
for trace_number in {1..64}; do
    # Check if trace_number is in the skip list
    if [[ " ${SKIP_TRACES[@]} " =~ " ${trace_number} " ]]; then
        echo "Skipping trace number ${trace_number}"
        continue
    fi

    echo "Processing trace number ${trace_number}"

    # Define log file paths
    LOG_FILE="${LOG_DIR}/trace_${trace_number}.log"
    ERR_FILE="${LOG_DIR}/trace_${trace_number}.err"

    # Navigate to the directory
    cd "$TRACE_DIR" || { echo "Failed to navigate to ${TRACE_DIR}" >>"$ERR_FILE"; continue; }

    # Download the trace file
    wget -O "cluster${trace_number}.sort.zst" "https://ftp.pdl.cmu.edu/pub/datasets/twemcacheWorkload/open_source/cluster${trace_number}.sort.zst" >>"$LOG_FILE" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Download failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

    # Decompress the trace file
    zstd -d "cluster${trace_number}.sort.zst" >>"$LOG_FILE" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Decompression failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

    rm "cluster${trace_number}.sort.zst"

    # Process the trace file
    pv "cluster${trace_number}.sort" | sed -n '1,1500000000p' >"cluster${trace_number}.sort_" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Processing failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

    # Remove the original trace file
    rm "cluster${trace_number}.sort"

    # Run output processing
    "${CODE_DIR}/output" "cluster${trace_number}.sort_" "${trace_number}" >>"$LOG_FILE" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Output processing failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        rm "cluster${trace_number}.sort_"
        continue
    fi

    # Clean up temporary file
    rm "cluster${trace_number}.sort_"

    # Run Python plot script
    python "${CODE_DIR}/plot.py" "./${trace_number}/freq.txt" "${trace_number}" >>"$LOG_FILE" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Plotting failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi

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
    ./run_cache_simulator.sh "$trace_number" "$DATASET_SIZE" "$CBA_UPDATE_INTERVAL" >>"$LOG_FILE" 2>>"$ERR_FILE"
    if [[ $? -ne 0 ]]; then
        echo "Cache simulator failed for trace ${trace_number}, skipping..." >>"$ERR_FILE"
        continue
    fi
    cd "$TRACE_DIR" || { echo "Failed to navigate to ${TRACE_DIR}" >>"$ERR_FILE"; continue; }

    echo "Trace ${trace_number} completed successfully" >>"$LOG_FILE"

done

echo "All traces processed."
