#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_filter_plugin.so"
TARGET="${abs_builddir}/app_filter"

echo "=== [Diagnostics] Running app_filter ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" "$@" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [ $EXIT_CODE -ne 0 ]; then
    echo "ERROR: Test failed with exit code $EXIT_CODE"
    exit $EXIT_CODE
fi

if [[ "$OUTPUT" == *"[AuditCore] WARNING: Diverging Chain Detected"* ]]; then
    echo "ERROR: Unexpected Dynamic Dispatch warning found. Filter test should use static chaining."
    exit 1
fi

exit 0