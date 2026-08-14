# Audit Hook Test Suite

This directory contains the integration tests for the `audit_hook` framework. These tests validate the core `LD_AUDIT` backend engine (`libaudit_core.so`) and the C++20 compile-time trampoline frontend.

Because `LD_AUDIT` libraries operate very early in the process lifecycle and within an isolated linker namespace, these tests are executed via shell scripts that inject the necessary environment variables (`LD_AUDIT` and `AH_PLUGIN`) into the target applications.

## Test Components

### Target Applications & Libraries
*   **`app_simple.c`**: A basic application that calls standard C library functions (like `puts`). Used to baseline wrapping and replacing behavior.
*   **`app_dlsym.c` & `dummy_lib.c`**: An application that explicitly loads a library (`dummy_lib.so`) via `dlopen` and manually resolves a function pointer using `dlsym`.
*   **`app_dlopen.c` & `libnum.c` / `libnum2.c`**: A complex test application that dynamically loads multiple libraries and verifies that symbol bindings across `dlopen` boundaries are successfully intercepted.

### Test Cases

#### 1. Function Replacement (`run_replace.sh`)
*   **Plugin:** `test_replace.cpp`
*   **Description:** Tests the `audit_hooks::register_replace` API. Validates that a target function (`puts`) can be entirely replaced with a zero-overhead hook. Ensures that the C++ standard library optimization (which converts `printf` without formatting into `puts`) does not cause infinite recursion when replacing `puts`.

#### 2. Function Wrapping (`run_wrap.sh`)
*   **Plugin:** `test_wrap.cpp`
*   **Description:** Tests the `audit_hooks::register_wrap` API. Validates that a target function (`puts`) can be intercepted, that the wrapper can safely execute its own logic, and that the wrapper can successfully call the original underlying function pointer.

#### 3. dlsym Interception (`run_dlsym.sh`)
*   **Plugin:** `test_dlsym_plugin.cpp`
*   **Description:** Validates that when a target application manually queries the dynamic linker for a function pointer via `dlsym(handle, "target_function")`, the audit engine intercepts the lookup and returns the C++ trampoline instead of the real function address. Ensures that the internal recursive `la_symbind` trigger caused by glibc's `dlsym` implementation is correctly bypassed.

#### 4. dlopen Chaining (`run_dlopen.sh`)
*   **Plugin:** `test_dlopen_plugin.cpp`
*   **Description:** Validates that functions loaded late into the process via `dlopen` are successfully hooked, and that `RTLD_DEFAULT` lookups return the wrapped functions. 
*   **Attribution:** This test case and its associated libraries (`libnum.c`, `libnum2.c`, `app_dlopen.c`) were derived directly from LLNL's GOTCHA framework test suite, specifically: [https://github.com/llnl/GOTCHA/test/dlopen](https://github.com/llnl/GOTCHA/test/dlopen).

#### 5. Caller-based Filtering (`run_filter.sh`)
*   **Plugin:** `test_filter_plugin.cpp`
*   **Description:** Validates that plugins can dynamically filter their own execution based on the identity of the calling library (emulating GOTCHA's `gotcha_filter_libraries_by_name`). It uses `dladdr` and `__builtin_return_address(1)` inside the C++ wrapper to bypass hooks if the call did not originate from the allowed shared object.
*   **Attribution:** This test case and its associated logic were derived from LLNL's GOTCHA framework test suite, specifically: [https://github.com/llnl/GOTCHA/tree/develop/test/filter](https://github.com/llnl/GOTCHA/tree/develop/test/filter).

## Core API & Integration Tests

The `test/` directory contains several integration tests that validate the core functionality of the `LD_AUDIT` backend engine and the C++20 API. Each test consists of a target application, a dummy library, a C++ plugin, and a bash script runner.

| Script | Plugin | What it validates |
| :--- | :--- | :--- |
| `run_replace.sh` | `test_replace.cpp` | **Pure Replacement:** Validates `register_replace` for zero-overhead function swapping. |
| `run_wrap.sh` | `test_wrap.cpp` | **Function Wrapping:** Validates `register_wrap` to intercept a function, execute custom logic, and seamlessly call the original underlying pointer. |
| `run_dlsym.sh` | `test_dlsym_plugin.cpp` | **dlsym Interception:** Validates that the framework successfully intercepts dynamic symbol lookups via `dlsym` and routes them to our trampolines. |
| `run_dlopen.sh` | `test_dlopen_plugin.cpp` | **Late-Bound dlopen:** Validates that hooks successfully attach to libraries loaded lazily via `dlopen` at runtime. |
| `run_filter.sh` | `test_filter_plugin.cpp` | **Basic Caller Filtering:** Validates that `ah_set_caller_filter` successfully isolates a hook to a specific calling library using `AH_FILTER_INCLUDE`. |
| `run_comp.sh` | `test_comp_plugin.cpp` | **Hook Composition:** Validates the chaining rules when multiple directives target the same function (Wrap $\rightarrow$ Replace, Replace $\rightarrow$ Wrap, Wrap $\rightarrow$ Wrap, Replace $\rightarrow$ Replace) and ensures the correct warnings are emitted. |
| `run_filter_state.sh` | `test_filter_state_plugin.cpp` | **Filter State Machine:** Validates the complex set-logic of caller filters (e.g., Include $\rightarrow$ Exclude set differences and their respective warnings). |

## Dynamic Dispatch & Filter Isolation Tests

The following tests validate the **Ordered Filtered Projections** model, ensuring that caller-specific filters isolate plugins correctly and trigger dynamic dispatch only when necessary. All three tests utilize shared dummy libraries (`libdispatch_target.so`, `libdispatch_x.so`, and `libdispatch_y.so`) to simulate distinct callers hitting the same target functions.

| Script | Plugins | What it validates |
| :--- | :--- | :--- |
| `run_test_divergent.sh` | `plugin_div_a.cpp`, `plugin_div_b.cpp` | **Divergent Wrapper Chain:** Proves that applying a caller-specific filter to a multiply-wrapped function correctly detects a conflict and triggers the dynamic dispatcher, routing `libX` and `libY` to different wrapper chains on the fly. |
| `run_test_static.sh` | `plugin_stat_a.cpp`, `plugin_stat_b.cpp` | **Pure Static Chain:** Proves that multiple global wrappers safely fall back to chaining via zero-overhead static pointers, correctly skipping the dynamic dispatcher. |
| `run_test_mixed.sh` | `plugin_mix_a.cpp`, `plugin_mix_b.cpp` | **Mixed Action Divergence:** Proves that a filtered wrapper correctly and safely dispatches into a globally replaced function (rather than the native OS function), resolving the mix of `register_wrap` and `register_replace` states based on the caller. |
| `run_fp_dynamic.sh` | `plugin_fp_dynamic.cpp` | **Runtime Filter Toggling (GOTCHA Function Pointer Adaptation):** Validates the `register_dynamic` API and namespace-bridging via the `libaudit_hook_dynamic.so` stub. Adapted from the GOTCHA function pointer test, it proves that a captured function pointer seamlessly reflects runtime-activated hooks (toggled via `ah_set_caller_filter`) without needing to be re-resolved by the OS. |

To compile and execute the test suite, run the following from the root directory:

```bash
make check

Detailed diagnostic output (including standard error, stdout, and exit codes) for any failing tests can be found in test/test-suite.log.
