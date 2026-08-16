# audit_hook — AGENTS.md

## Build & Test

```bash
autoreconf -i && ./configure && make && make check   # from repo root only
```

`make check` runs 12 test scripts under `test/TESTS`. Diagnostics go to `test/test-suite.log`. Tests use `abs_top_builddir` / `LD_LIBRARY_PATH` env vars injected via `AM_TESTS_ENVIRONMENT` in `test/Makefile.am` — never run tests manually from a subdirectory.

## Architecture

Two-level autotools tree: `SUBDIRS = src test`.

- **Root Makefile.am** — delegates to SUBDIRS, installs `include/audit_hook.hpp` and `include/audit_hook_dynamic.h` via `include_HEADERS`.
- **`src/Makefile.am`** — builds two libtool libraries:
  - `libaudit_core.la` — LD_AUDIT engine (C++20: concepts, ranges, constexpr, thread-local state). Hooks stored in a mutex-guarded `unordered_map`; caller identity tracked via `la_objopen`/`la_objclose` cookie-to-pathname mapping.
  - `libaudit_hook_dynamic.la` — namespace bridge stub allowing target apps (in the main executable's address space) to call into the isolated LD_AUDIT engine via `La_symbind64` interception.
- **`test/Makefile.am`** — 23 shared-lib plugins (via `check_LTLIBRARIES`), 10 test executables (`check_PROGRAMS`), 12 shell-test scripts (`TESTS`). All `.libs/*.so` files are plain `.so` with no soname or ABI version suffix.

## Build system constraints

- **`CXXFLAGS += -std=c++20`** — non-trivial: if the compiler defaults to C++17, the build fails silently until linking (template errors cascade from concepts).
- **Autoreconf required after editing any `.ac`, `.am`, or m4 file.** After autoreconf, re-run `configure` (it is not incremental).
- **rpath set to `${abs_builddir}`** on every plugin so `LD_AUDIT=libaudit_core.so` finds it at runtime without installing.
- **libtool `.libs/` artifacts**: built binaries sit in `test/.libs/`. Tests launch them directly (not the `test/` root) to avoid `LD_AUDIT` recursively auditing libtool's bash helpers.
- **`AH_PLUGINS=` colon-delimited** — newer API. Old `$AH_PLUGIN` (singular) is deprecated by the C++ code.
- **Static chaining vs dynamic dispatch**: When multiple plugins wrap the same symbol with matching or global filters, hooks chain statically at link-time. Conflicting caller filters trigger automatic dynamic dispatch via TLS + `dladdr`. This upgrade emits a stderr warning.

## Adding a test (plugin)

In `test/Makefile.am`, add to all three lists:
1. `check_LTLIBRARIES` — plugin `.la` rule with `LDFLAGS = $(PLUGIN_LIBS)` (which pulls in `$(top_builddir)/src/libaudit_core.la`)
2. `check_PROGRAMS` — target executable with its `_LDADD`
3. `TESTS` — shell runner script, listed under the `EXTRA_DIST` section that is already defined

The runner must:
- Source `abs_top_builddir` and `abs_builddir` from env (or use the test-driver defaults)
- Export `LD_LIBRARY_PATH="${abs_builddir}/.libs:${abs_top_builddir}/src/.libs"`
- Launch `${abs_builddir}/.libs/<test_binary>` with `LD_AUDIT=<core.so> AH_PLUGINS=<plugin.so>` and pipe stderr through stdout
- Assert output contains test markers (usually `SUCCESS`)

## Hook & filter semantics (quick reference)

### Composition (same symbol, multiple plugins)

| Sequence | Behavior | Warning? |
|---|---|---|
| Wrap → Replace | Discard wrap; replace takes over | Yes |
| Replace → Wrap | Chain: replaced func becomes wrapper "original" | No |
| Wrap → Wrap | Chain outward → inward → native OS | No |
| Replace → Replace | Overwrite; new replace wins | Yes |

### Filters (per-hook, `ah_set_caller_filter`)

| Current state | Directive | Result | Duplicates? |
|---|---|---|---|
| Global → Include | Full replacement of global | — |
| Include+Include | Union to existing list | Silently ignored |
| Global → Exclude | All except listed libs | — |
| Exclude+Exclude | Union to exclusion list | Silently ignored |
| Include−Exclude | Set-difference (remove listed) | Warn if not present |
| Exclude∪Include | Inverted set-diff (re-permit) | Warn if not present |

## Developer notes

- clang-format config is in `.clang-format` — run `clang-format -i <file>` before committing.
- CI/lint setup: none detected in the repo. No Makefile target for formatting, only build & test.
