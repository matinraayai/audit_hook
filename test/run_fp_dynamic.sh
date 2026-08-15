#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libplugin_fp_dynamic.so"
TARGET="${abs_builddir}/app_fp_dynamic"

echo "=== [Diagnostics] Running app_fp_dynamic ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

exit 0