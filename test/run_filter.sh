#!/bin/sh

# 1. Unset any inherited plugin variables to prevent leakage from other tests
unset AH_PLUGINS
unset AH_PLUGIN

# 2. Explicitly define ONLY the plugin needed for this test
export AH_PLUGINS="./test_filter_plugin.so"

# 3. Set the audit core library
export LD_AUDIT="libaudit_core.so"

# 4. Run the target application and pass along any script arguments
exec ./app_filter "$@"