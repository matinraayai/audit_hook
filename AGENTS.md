# audit_hook — AGENTS.md

## Build & Test

```bash
autoreconf -i && ./configure && make && make check
```

`make check` runs 5 integration tests via shell scripts in `test/`. Diagnostics go to `test/test-suite.log`.

## Architecture

- **`src/audit_core.cpp`** — the LD_AUDIT backend engine (C++20, compiled to `libaudit_core.so`)
- **`include/audit_hook.hpp`** — public C++20 API header, shipped with `make install`
- **`test/test_*.cpp`** — plugin wrappers that exercise each API variant

5 test plugins map 1:1 to test scripts:

| Script | Plugin | What it validates |
|---|---|---|
| `run_replace.sh` | `test_replace.cpp` | `register_replace` (zero-overhead function swap) |
| `run_wrap.sh` | `test_wrap.cpp` | `register_wrap` (intercept + call original) |
| `run_dlsym.sh` | `test_dlsym_plugin.cpp` | dlsym lookup interception |
| `run_dlopen.sh` | `test_dlopen_plugin.cpp` | late-bound dlopen hooks, RTLD_DEFAULT |
| `run_filter.sh` | `test_filter_plugin.cpp` | caller-based include/exclude filtering |

The non-C++ sources in `test/` (`app_*.c`, `lib*.c`) are shim binaries/libraries that exercise the plugins. Do not treat them as library code — they are test harness fixtures.

## Writing a plugin

Plugins must link against `src/libaudit_core.la`. From Makefile.am:

```
PLUGIN_LIBS = -module -shared -avoid-version -rpath $(abs_builddir) $(top_builddir)/src/libaudit_core.la
```

They need `-fPIC` and `-I$(top_srcdir)/include`. See `test/Makefile.am` for a working template.

## Key constraints

- **C++20 required** (`configure.ac:11`). Features used: concepts, ranges, `constexpr`, templates, thread-local storage.
- **No libtool versioning** — all shared libs use `-avoid-version`; they produce plain `.so` with no soname suffixes.
- **rpath matters** — test plugins and libraries set `-rpath $(abs_builddir)` so `LD_AUDIT=libaudit_core.so` finds them at runtime without installing.
- **`make check` only from the root directory** — the shell scripts use relative paths that depend on the top-level build layout.
- **Autoreconf regenerator** — after editing `configure.ac`, `Makefile.am`, or m4 macros, run `autoreconf -i` before `./configure`.

## Known gaps

See TODO.md — unresolved: filter test coverage for include vs exclude mode.
