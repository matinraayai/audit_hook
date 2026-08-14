#!/bin/bash
export LD_LIBRARY_PATH="$(pwd)/.libs:../src/.libs:$LD_LIBRARY_PATH"

echo "=== Capturing Natively Executed Baseline ==="
unset LD_AUDIT
unset AH_PLUGINS
unset HAMMER_MODE

./app_hammer
if [ $? -ne 0 ]; then
    echo "ERROR: Baseline execution failed."
    exit 1
fi

had_error=0

# Helper function to run and verify a specific plugin mode
verify_hammer_mode() {
    local MODE=$1
    echo -e "\n=== Testing Mode: $MODE ==="
    
    export LD_AUDIT="../src/.libs/libaudit_core.so"
    export AH_PLUGINS="$(pwd)/.libs/libplugin_hammer.so"
    export HAMMER_MODE="$MODE"

    ./app_hammer
    if [ $? -ne 0 ]; then
        echo "ERROR: App execution failed under LD_AUDIT in mode $MODE."
        had_error=1
    fi
}

# Run both modes
verify_hammer_mode "mult"
verify_hammer_mode "add"

echo ""
if [ "$had_error" -eq 0 ]; then
    echo "SUCCESS: All Hammer modes successfully wrapped and verified!"
    exit 0
else
    echo "FAIL: One or more Hammer modes failed verification."
    exit 1
fi