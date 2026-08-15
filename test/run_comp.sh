#!/bin/bash
unset AH_PLUGINS
unset AH_PLUGIN

: "${abs_top_builddir:=..}"
: "${abs_builddir:=.}"

CORE="${abs_top_builddir}/src/.libs/libaudit_core.so"
PLUGIN="${abs_builddir}/.libs/libtest_comp_plugin.so"
TARGET="${abs_builddir}/app_comp"

echo "=== [Diagnostics] Running app_comp ==="
OUTPUT=$(LD_AUDIT="$CORE" AH_PLUGINS="$PLUGIN" "$TARGET" 2>&1)
EXIT_CODE=$?

echo "$OUTPUT"

if [[ "$OUTPUT" != *"SUCCESS"* ]] || [ $EXIT_CODE -ne 0 ]; then
    exit 1
fi

# Verify exactly the right warnings were emitted
if [[ "$OUTPUT" != *"[AuditCore] WARNING: Symbol 'func_w_r' was wrapped by 'tool_wrap', but is now being entirely replaced by 'tool_replace'"* ]]; then
    echo "ERROR: Missing Wrap->Replace warning."
    exit 1
fi

if [[ "$OUTPUT" != *"[AuditCore] WARNING: Symbol 'func_r_r' was already replaced by 'tool_replace1', but is now being replaced again by 'tool_replace2'"* ]]; then
    echo "ERROR: Missing Replace->Replace warning."
    exit 1
fi

exit 0