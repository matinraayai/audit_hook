#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN_A="${abs_builddir}/.libs/libplugin_mix_a.so"
PLUGIN_B="${abs_builddir}/.libs/libplugin_mix_b.so"
TARGET="${abs_builddir}/app_test_mixed"

echo "=== [Diagnostics] Running app_test_mixed ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="${PLUGIN_A}:${PLUGIN_B}" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

if [[ "$OUTPUT" != *"[AuditCore] WARNING: Diverging Chain Detected"* ]]; then
    echo "ERROR: Dynamic Dispatch warning was suppressed."
    exit 1
fi

exit 0