#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN="./.libs/libtest_replace.so"

echo "=== [Diagnostics] Checking Libraries ==="
ls -l $CORE || echo "MISSING CORE!"
ls -l $PLUGIN || echo "MISSING PLUGIN!"

echo "=== [Diagnostics] Running App ==="
# Inject the variables inline ONLY into app_simple!
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" ./app_simple 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: Function was Replaced!"* ]]; then
    exit 0
else
    exit 1
fi