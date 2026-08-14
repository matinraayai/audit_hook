#!/bin/bash
CORE="../src/.libs/libaudit_core.so"
PLUGIN="./.libs/libplugin_fp_dynamic.so"

echo "=== [Diagnostics] Running app_fp_dynamic ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" ./app_fp_dynamic 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

exit 0