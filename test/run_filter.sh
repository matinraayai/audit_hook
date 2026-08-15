#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

# Manually point the linker to our uninstalled libraries so the raw binary can run
export LD_LIBRARY_PATH="${abs_builddir}/.libs:${abs_top_builddir}/src/.libs:$LD_LIBRARY_PATH"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_filter_plugin.so"

# Execute the raw binary in .libs/ to prevent LD_AUDIT from 
# recursively auditing Libtool's bash utilities!
TARGET="${abs_builddir}/.libs/app_filter"

echo "=== [Diagnostics] Running app_filter ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" "$@" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

# Simply check for the SUCCESS string and a clean exit code
if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    echo "ERROR: Test failed."
    exit 1
fi

exit 0