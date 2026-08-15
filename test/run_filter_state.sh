#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_filter_state_plugin.so"
TARGET="${abs_builddir}/app_filter_state"

echo "=== [Diagnostics] Running app_filter_state ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

# Verify exactly the right warnings were emitted for the Set Difference rules
if [[ "$OUTPUT" != *"[AuditCore] WARNING: Tool 'tool1' attempted to exclude library 'libB.so' on symbol 'func1', but it was not in the active INCLUDE list"* ]]; then
    echo "ERROR: Missing Include->Exclude set difference warning."
    exit 1
fi

if [[ "$OUTPUT" != *"[AuditCore] WARNING: Tool 'tool2' attempted to include library 'libB.so' on symbol 'func2', but it was not in the active EXCLUDE list"* ]]; then
    echo "ERROR: Missing Exclude->Include inverted set difference warning."
    exit 1
fi

exit 0