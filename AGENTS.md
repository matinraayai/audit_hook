# audit_hook — AGENTS.md

## Build & Test

```bash
autoreconf -i && ./configure && make && make check
```

`make check` runs 7 integration tests via shell scripts in `test/`. Diagnostics go to `test/test-suite.log`.

## Architecture

- **`src/audit_core.cpp`** — LD_AUDIT backend engine (C++20). Hooks stored in a magic-static `unordered_map`; caller identity tracked via `la_objopen`/`la_objclose` cookie→pathname map. Captures real `dlsym` during `la_symbind64/32`.
- **`include/audit_hook.hpp`** — public C++20 API header (shipped via `make install`). Provides `ah_register_hook`, `ah_set_caller_filter`, thread pause/resume/ignore functions, and the `audit_hooks::register_wrap` / `register_replace` templates.
- **`test/test_*.cpp`** — 7 plugin wrappers exercising each API variant (see table below).

Test plugins are one-to-one with test scripts:

| Script | Plugin | What it validates |
|---|---|---|
| `run_replace.sh` | `test_replace.cpp` | `register_replace` (zero-overhead function swap) |
| `run_wrap.sh` | `test_wrap.cpp` | `register_wrap` (intercept + call original) |
| `run_dlsym.sh` | `test_dlsym_plugin.cpp` | dlsym lookup interception |
| `run_dlopen.sh` | `test_dlopen_plugin.cpp` | late-bound dlopen hooks, RTLD_DEFAULT |
| `run_filter.sh` | `test_filter_plugin.cpp` | caller-based include/exclude filtering (include-only) |
| `run_comp.sh` | `test_comp_plugin.cpp` | hook composition: wrap→replace, replace→wrap, wrap→wrap, replace→replace chaining rules |
| `run_filter_state.sh` | `test_filter_state_plugin.cpp` | filter state machine: include/exclude set-difference with ordering |

Non-C++ sources in `test/` (`app_*.c`, `lib*.c`) are shim binaries/libraries — do not treat them as library code.

## Writing a plugin

Plugins must link against `src/libaudit_core.la`. From `test/Makefile.am`:

```
PLUGIN_LIBS = -module -shared -avoid-version -rpath $(abs_builddir) $(top_builddir)/src/libaudit_core.la
```

Need `-fPIC` and `-I$(top_srcdir)/include`. See `test/Makefile.am` for a working template.

## Running with plugins

```bash
AH_PLUGINS=./plugin1.so:./plugin2.so LD_AUDIT=libaudit_core.so ./target_app
```

Plugins are colon-delimited, loaded sequentially in `la_preinit` via `dlopen(RTLD_NOW | RTLD_LOCAL)`. The older single `AH_PLUGIN` env var is replaced — use `AH_PLUGINS`.

## Hook composition semantics

When multiple plugins register hooks for the same symbol (loaded in sequence), they resolve deterministically:

| Sequence | Action | Warning? |
|---|---|---|
| Wrap → Replace | Discard wrap; new replace takes over | Yes, to stderr |
| Replace → Wrap | Chain: replaced function becomes the "original" of the wrapper | No |
| Wrap → Wrap | Chain: outermost calls inner which calls native OS function | No |
| Replace → Replace | Overwrite old replace; new replace takes over | Yes, to stderr |

## Filter state machine

`ah_set_caller_filter` maintains persistent include/exclude lists per hook across subsequent calls. New registrations are treated as set operations on the current state:

| Current state | Directive | Result | Duplicates? |
|---|---|---|---|
| Global | Include | Replace with include list | — |
| Include | Include (union) | Append new libs to include list | Silently ignored |
| Global | Exclude | All except listed libs | — |
| Exclude | Exclude (union) | Append new libs to exclude list | Silently ignored |
| Include | Exclude (set diff) | Remove listed libs from include list | Warn if not in current include list |
| Exclude | Include (inverted set diff) | Remove listed libs from exclude list (re-permit them) | Warn if not in current exclude list |

## Key constraints

- **C++20 required** (`configure.ac:11`). Features used: concepts, ranges, `constexpr`, templates, thread-local storage.
- **No libtool versioning** — all shared libs use `-avoid-version`; they produce plain `.so` with no soname suffixes.
- **rpath matters** — test plugins and libraries set `-rpath $(abs_builddir)` so `LD_AUDIT=libaudit_core.so` finds them at runtime without installing.
- **`make check` only from the root directory** — shell scripts use relative paths that depend on the top-level build layout.
- **Autoreconf regenerator** — after editing `configure.ac`, `Makefile.am`, or m4 macros, run `autoreconf -i` before `./configure`.

## Developer notes

- The filter test (`run_filter.sh`) now validates both include-only and set-difference cases (TODO resolved).
- If adding a new plugin test: update `check_PROGRAMS`, `check_LTLIBRARIES`, and `TESTS` in `test/Makefile.am`; the test script should check both app output AND stderr for expected `[AuditCore] WARNING:` or `[AuditCore] FATAL:` messages.
