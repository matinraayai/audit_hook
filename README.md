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
