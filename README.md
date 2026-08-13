# Audit Hook API

A high-performance, type-safe API for wrapping and replacing C/C++
functions dynamically. It provides the developer ergonomics of
`LD_PRELOAD` and GOTCHA, but is backed natively by Linux's `LD_AUDIT`
linker interface.

## Features
* **Zero dlsym bootstrap overhead**: The linker hands us the original
  function pointer.
* **C++20 Compile-time Trampolines**: Auto-manages thread-local state
  to prevent recursive loops.
* **Native dlopen/dlsym support**: Automatically hooks dynamically
  loaded libraries.

## Build Instructions

bash
autoreconf -i
./configure
make
make check


## Usage
Link your plugin against `libaudit_core.so` and use
`audit_hooks::register_wrap`.  Run your target application with:

bash
AH_PLUGIN=./my_plugin.so LD_AUDIT=libaudit_core.so ./target_app

*(Or compile your app with `-Wl,--audit=libaudit_core.so` to avoid
setting LD_AUDIT)*

## Hook Composition Semantics

When multiple plugins (loaded sequentially via `AH_PLUGINS`) or multiple directives target the exact same function, the framework resolves them using a predictable chaining state machine.

*   **Wrap followed by Replace:** The existing wrapping is completely discarded in favor of the new replacement. A warning is emitted to `stderr` to notify the user of the override.
*   **Replace followed by Wrap:** The replaced function is treated as the underlying implementation, and the new wrapper successfully wraps it.
*   **Wrap followed by Wrap (Chaining):** The wrappers are chained. The outermost (newest) wrapper will call the inner (older) wrapper, which ultimately calls the native OS function.
*   **Replace followed by Replace:** The older replacement is completely overwritten by the newer replacement. A warning is emitted to `stderr` to notify the user of the override.

## Caller Filter Semantics

Caller filters (`ah_set_caller_filter`) allow tools to limit hooks based on the origin of the call using `AH_FILTER_GLOBAL`, `AH_FILTER_INCLUDE`, and `AH_FILTER_EXCLUDE`. The state machine elegantly resolves overlapping or conflicting directives:

*   **Default State:** When a function is first hooked, its filter defaults to `AH_FILTER_GLOBAL` (the hook applies to all callers).
*   **Global to Include:** The `AH_FILTER_GLOBAL` state is replaced. The hook will now only apply to the libraries specified in the inclusion list.
*   **Include to Include (Union):** New libraries are appended to the existing inclusion list. Duplicates are silently ignored.
*   **Global to Exclude:** The `AH_FILTER_GLOBAL` state is retained in spirit, but the hook will now apply to all libraries *except* those specified in the exclusion list.
*   **Exclude to Exclude (Union):** New libraries are appended to the existing exclusion list. Duplicates are silently ignored.
*   **Include to Exclude (Set Difference):** Any currently included libraries specified in the new exclude directive are removed from the active hook list. If a library is excluded but was not currently on the inclusion list, a warning is emitted.
*   **Exclude to Include (Inverted Set Difference):** Any currently excluded libraries specified in the new include directive are removed from the active exclusion list (thereby permitting them again). If a library is included but was not currently on the exclusion list, a warning is emitted.
