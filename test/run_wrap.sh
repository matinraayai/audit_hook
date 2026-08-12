#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN="./.libs/libtest_wrap.so"

echo "=== [Diagnostics] Running App ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGIN="$PLUGIN" ./app_simple 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: Function was Wrapped!"* ]] && [[ "$OUTPUT" == *"Target App: Original Execution"* ]]; then
    exit 0
else
    exit 1
fi