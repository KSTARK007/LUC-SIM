#!/bin/bash

# Define window percentages, workload numbers, and total keys & operations
WINDOW_PCTS=(1 2 5 10 15 20 25)       # Percentages of total keys
# WINDOW_PCTS=(10)       # Percentages of total keys
# WORKLOAD_NUMBERS=(7)      # Workload dataset numbers
# TOTAL_KEYS=4467939              # Total keys available
# TOTAL_OPS=851370550             # Total operations
WORKLOAD_NUMBERS=(19)      # Workload dataset numbers
TOTAL_KEYS=328491867              # Total keys available
TOTAL_OPS=1503296677             # Total operations

# Directory to store logs
LOG_DIR="cba_simulation_logs"
mkdir -p "$LOG_DIR"

# Path to the compiled C++ binary
EXECUTABLE="./test_cdf"

# Log file to store screen session names
SCREEN_LOG_FILE="$LOG_DIR/screen_sessions.log"
echo "Tracking screen sessions:" > "$SCREEN_LOG_FILE"

# Run simulations in parallel using screen
for WINDOW_PCT in "${WINDOW_PCTS[@]}"; do
    for WORKLOAD in "${WORKLOAD_NUMBERS[@]}"; do
        # Calculate the window size as a percentage of total keys
        WINDOW_SIZE=$((TOTAL_KEYS * WINDOW_PCT / 100))

        SCREEN_NAME="sim_wp${WINDOW_PCT}_wl${WORKLOAD}"
        LOG_FILE="${LOG_DIR}/${SCREEN_NAME}.log"

        # Start a new screen session and run the command
        screen -dmS "$SCREEN_NAME" bash -c "$EXECUTABLE $WINDOW_PCT $WORKLOAD $TOTAL_KEYS $TOTAL_OPS > $LOG_FILE 2>&1"

        # Log screen session names
        echo "$SCREEN_NAME" >> "$SCREEN_LOG_FILE"

        echo "Started screen session '$SCREEN_NAME' for window percentage $WINDOW_PCT% and workload $WORKLOAD. Logs: $LOG_FILE"
    done
done

echo "All simulations have been started in parallel."
echo "Screen session names are logged in $SCREEN_LOG_FILE."
