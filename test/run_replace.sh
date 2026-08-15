#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_replace.so"
TARGET="${abs_builddir}/app_simple"

echo "=== [Diagnostics] Checking Libraries ==="
ls -l "$CORE" || echo "MISSING CORE!"
ls -l "$PLUGIN" || echo "MISSING PLUGIN!"

echo "=== [Diagnostics] Running App ==="
# Inject the variables inline ONLY into app_simple!
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "=== [Diagnostics] Output ==="
echo "$OUTPUT"
echo "=== [Diagnostics] Exit Code: $EXIT_CODE ==="

if [[ "$OUTPUT" == *"SUCCESS: Function was Replaced!"* ]]; then
    exit 0
else
    exit 1
fi