#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN="./.libs/libtest_dlopen_plugin.so"

echo "=== [Diagnostics] Running app_dlopen ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGIN="$PLUGIN" ./app_dlopen 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: All dlopen and dlsym tests passed!"* ]] && [ $EXIT_CODE -eq 0 ]; then
    exit 0
else
    exit 1
fi