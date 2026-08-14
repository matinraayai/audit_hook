#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN_A="./.libs/libplugin_stat_a.so"
PLUGIN_B="./.libs/libplugin_stat_b.so"

echo "=== [Diagnostics] Running app_test_static ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN_A:$PLUGIN_B" ./app_test_static 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

if [[ "$OUTPUT" == *"[AuditCore] WARNING: Diverging Chain Detected"* ]]; then
    echo "ERROR: Dynamic Dispatch warning was accidentally triggered."
    exit 1
fi

exit 0