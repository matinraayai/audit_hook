#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN="./.libs/libtest_dlsym_plugin.so"

echo "=== [Diagnostics] Running App ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" ./app_dlsym 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: Intercepted"* ]] && [[ "$OUTPUT" == *"val=142"* ]]; then
    exit 0
else
    exit 1
fi