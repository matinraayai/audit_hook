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

## Running the Tests

To compile and execute the test suite, run the following from the root directory:

```bash
make check

Detailed diagnostic output (including standard error, stdout, and exit codes) for any failing tests can be found in test/test-suite.log.
