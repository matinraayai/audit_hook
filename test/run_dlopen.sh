#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_dlopen_plugin.so"
TARGET="${abs_builddir}/app_dlopen"

echo "=== [Diagnostics] Running app_dlopen ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: All dlopen and dlsym tests passed!"* ]] && [ $EXIT_CODE -eq 0 ]; then
    exit 0
else
    exit 1
fi