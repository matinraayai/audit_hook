#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_dlsym_plugin.so"
TARGET="${abs_builddir}/app_dlsym"

echo "=== [Diagnostics] Running App ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: Intercepted"* ]] && [[ "$OUTPUT" == *"val=142"* ]]; then
    exit 0
else
    exit 1
fi