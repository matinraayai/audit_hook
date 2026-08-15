#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

# Need to include the .libs dir for dynamic lookup of libraries the app might use
export LD_LIBRARY_PATH="${abs_builddir}/.libs:${abs_top_builddir}/src/.libs:$LD_LIBRARY_PATH"

TARGET="${abs_builddir}/app_hammer"

echo "=== Capturing Natively Executed Baseline ==="
unset LD_AUDIT
unset HAMMER_MODE

"$TARGET"
if [ $? -ne 0 ]; then
    echo "ERROR: Baseline execution failed."
    exit 1
fi

had_error=0

# Helper function to run and verify a specific plugin mode
verify_hammer_mode() {
    local MODE=$1
    echo -e "\n=== Testing Mode: $MODE ==="
    
    local CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
    local PLUGIN="${abs_builddir}/.libs/libplugin_hammer.so"
    
    # Use inline execution to prevent auditing bash itself
    HAMMER_MODE="$MODE" LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET"
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