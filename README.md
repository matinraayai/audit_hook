# Audit Hook API

A high-performance, type-safe API for wrapping and replacing C/C++ functions dynamically. It provides the developer ergonomics of `LD_PRELOAD` and GOTCHA, but is backed natively by Linux's `LD_AUDIT` linker interface. `audit_hook` delivers the simplicity of traditional interception techniques without the fragility, ABI restrictions, or architecture-specific hacks that plague older methods.

## Motivation & Background

Dynamic function interception is a critical capability for performance profiling, debugging, and security tools. However, traditional approaches suffer from significant limitations:

### The Problem with `LD_PRELOAD`

While `LD_PRELOAD` is simple to use and widely understood, it is notoriously fragile and frequently results in unintended consequences. Relying on `LD_PRELOAD` introduces several structural and operational limitations:

* **Tool Collisions:** `LD_PRELOAD` becomes extraordinarily problematic when multiple tools attempt to preload and intercept the same functions simultaneously. Because there is no native mechanism for safely chaining hooks, this usually leads to unpredictable behavior, infinite recursion, or immediate crashes.
* **Strict ABI Constraints:** Any preloaded tool must be strictly ABI-compatible with the target application and all of its dependencies. This forces tool developers to maintain and distribute multiple versions of their tool for every conceivable ABI combination. Consequently, users are left with the difficult task of manually selecting the exact correct tool version to match their specific application environment.
* **Inability to Hook Statically Linked Code:** `LD_PRELOAD` operates entirely by intercepting dynamic symbol resolution. If a target application statically links a library or specific functions, `LD_PRELOAD` is completely blind to them and cannot intercept the calls.
* **Bypassed Internal Library Calls:** Standard libraries (like `glibc`) frequently bypass the Procedure Linkage Table (PLT) when making internal calls to their own functions for performance reasons. Because `LD_PRELOAD` relies on overriding the symbol resolution table, it completely fails to intercept these internal calls.
* **Security Restrictions (Setuid/Setgid):** For security reasons, the Linux dynamic linker strictly ignores `LD_PRELOAD` for `setuid` and `setgid` binaries. This makes it impossible to use preload-based tools on elevated applications without making invasive system trust policy changes.
* **Initialization Conflicts:** `LD_PRELOAD` libraries are injected incredibly early in the process lifecycle. If a tool requires complex initialization (such as allocating memory via `malloc` or initializing threading primitives) before the application and standard library are fully bootstrapped, it can easily deadlock or crash the process before `main()` even executes.

### The Problem with GOTCHA

`LD_AUDIT` provides a vastly superior, safer tooling interface provided directly by the dynamic linker. However, its raw API is notoriously difficult to implement correctly. Developers want the simplicity of `LD_PRELOAD` without the complexity of `LD_AUDIT`.

Tools like LLNL's GOTCHA attempted to bridge this gap by providing a simpler API, but they introduced their own severe drawbacks:

* **Hacky Memory Manipulation:** Because GOTCHA operates directly on the Global Offset Table (GOT), it relies on highly invasive techniques. It modifies the memory protection status of the pages containing the GOT table and literally rewrites the GOT entries in memory.
* **Poor Portability:** Manually manipulating the GOT makes GOTCHA extremely difficult to port across different CPU architectures.
* **Application Modifications:** GOTCHA operates inside the application itself, meaning target applications often require modifications or build-script changes just to use the tool.
* **ABI Constraints:** Like `LD_PRELOAD`, GOTCHA still suffers from ABI compatibility issues.

### The `audit_hook` Solution

`audit_hook` bridges the gap by leveraging the robust `LD_AUDIT` interface under the hood while exposing an API that is as simple to use as `LD_PRELOAD`.

* **Maximum Portability:** Because it uses the native audit interface, `ld.so` handles all the architecture-specific code and GOT table maintenance. `audit_hook` is inherently portable to any architecture that Linux's `LD_AUDIT` supports.
* **Zero Application Changes:** It functions completely transparently. It does not require any application source code modifications, recompilation, or changes to build scripts.
* **Advanced Capabilities:** It introduces powerful new capabilities impossible in standard `LD_PRELOAD`, such as targeted caller filtering and deterministic multi-tool composition.

Here is an expanded section for your `README.md` that highlights the modern C++ type safety features. You can add this directly under the **Motivation & Background** section or as a dedicated subsection under **Features**.

---

### Uncompromising Type Safety via Modern C++20

Traditional dynamic interception tools rely heavily on raw `void*` casting, unsafe macros, and `dlsym` type-punning. This bypasses the compiler's type checking entirely, meaning signature mismatches between the original function and the wrapper are not caught until they cause a segmentation fault at runtime.

`audit_hook` completely eliminates this class of errors by heavily leveraging C++20 features to guarantee absolute type safety at compile time:

* **Non-Type Template Parameters (NTTP):** The registration API uses `template <auto HookFunc, auto OriginalPtr>` to bind the hook and the original function pointer. This forces the C++ compiler to automatically deduce and verify both signatures during compilation. If your wrapper's arguments or return type do not perfectly match the target function, the code simply will not compile.


* **Compile-Time Trampoline Generation:** Instead of relying on fragile assembly thunks, `libffi`, or macro expansions, the framework's internal `HookGenerator` uses variadic templates (`typename... Args`) and `decltype` to automatically generate type-perfect trampolines and dispatchers. The arguments are perfectly forwarded to the underlying functions.


* **Compile-Time Branching (`if constexpr`):** Handling functions that return `void` versus those that return values is a notorious pain point in C-based interception tools. `audit_hook` elegantly handles this using `if constexpr (std::is_void_v<Ret>)`, generating the strictly correct return logic for each specific hook at compile time without any runtime overhead.


* **C++20 Concepts and Ranges:** The dynamic configuration API uses C++20 Concepts (`std::ranges::forward_range` and `requires std::convertible_to`) to enforce that caller filter lists are valid, contiguous collections of C-strings (`const char*`). This guarantees that dynamically passing `std::vector` or `std::array` configurations to the framework's state machine is rigorously type-checked at the boundary.


## Features

* **Zero dlsym bootstrap overhead**: The linker hands us the original function pointer.


* **C++20 Compile-time Trampolines**: Auto-manages thread-local state to prevent recursive loops.


* **Native dlopen/dlsym support**: Automatically hooks dynamically loaded libraries.


* **Ordered Filtered Projections**: Plugin actions are completely isolated. Hooks are evaluated per-caller using zero-overhead static chaining or automatic dynamic dispatch.



## Build Instructions

```bash
autoreconf -i
./configure
make
make check

```

## Usage

Link your plugin against `libaudit_core.so` and use `audit_hooks::register_wrap`. Run your target application with:

```bash
AH_PLUGINS=./my_plugin.so LD_AUDIT=libaudit_core.so ./target_app

```

*(Or compile your app with `-Wl,--audit=libaudit_core.so` to avoid setting LD_AUDIT)*

## Hook Composition Semantics

When multiple plugins (loaded sequentially via `AH_PLUGINS`) or multiple directives target the exact same function, the framework resolves them using a predictable chaining state machine. Each plugin's actions are isolated.

* **Wrap followed by Replace:** The existing wrapping is completely discarded in favor of the new replacement. A warning is emitted to `stderr` to notify the user of the override.


* **Replace followed by Wrap:** The replaced function is treated as the underlying implementation, and the new wrapper successfully wraps it.


* **Wrap followed by Wrap (Chaining):** The wrappers are chained. The outermost (newest) wrapper will call the inner (older) wrapper, which ultimately calls the native OS function.


* **Replace followed by Replace:** The older replacement is completely overwritten by the newer replacement. A warning is emitted to `stderr` to notify the user of the override.



## Caller Filter Semantics

Caller filters (`ah_set_caller_filter`) allow tools to limit hooks based on the origin of the call using `AH_FILTER_GLOBAL`, `AH_FILTER_INCLUDE`, and `AH_FILTER_EXCLUDE`. The state machine elegantly resolves overlapping or conflicting directives within a single tool:

* **Default State:** When a function is first hooked, its filter defaults to `AH_FILTER_GLOBAL` (the hook applies to all callers).


* **Global to Include:** The `AH_FILTER_GLOBAL` state is replaced. The hook will now only apply to the libraries specified in the inclusion list.


* **Include to Include (Union):** New libraries are appended to the existing inclusion list. Duplicates are silently ignored.


* **Global to Exclude:** The `AH_FILTER_GLOBAL` state is retained in spirit, but the hook will now apply to all libraries *except* those specified in the exclusion list.


* **Exclude to Exclude (Union):** New libraries are appended to the existing exclusion list. Duplicates are silently ignored.


* **Include to Exclude (Set Difference):** Any currently included libraries specified in the new exclude directive are removed from the active hook list. If a library is excluded but was not currently on the inclusion list, a warning is emitted.


* **Exclude to Include (Inverted Set Difference):** Any currently excluded libraries specified in the new include directive are removed from the active exclusion list (thereby permitting them again). If a library is included but was not currently on the exclusion list, a warning is emitted.



## Multi-Plugin Isolation & Dynamic Dispatch

The framework uses an **Ordered Filtered Projection** model to guarantee that filters applied by one plugin do not corrupt or bypass the chains of another plugin.

* **Static Chaining (Zero Overhead):** By default, if multiple plugins wrap the same function and their filters agree (or are entirely global), the dynamic linker resolves the stack of trampolines once at link-time. The framework hardcodes the chain into the static C++ `original_out` pointers with zero runtime overhead.


* **Diverging Chains & Dynamic Dispatch:** If multiple plugins wrap a function but apply *conflicting caller filters*, the framework detects a "Diverging Chain". Because a single static pointer cannot route differently based on who is calling, the framework automatically upgrades the chain to use **Dynamic Dispatch**.


* A loud warning is emitted to `stderr` indicating that dynamic dispatch is active.


* The frontend seamlessly replaces the raw function pointer call with a heavily optimized `Dispatcher` thunk.


* The dispatcher pushes the caller's address to a Thread-Local Storage (TLS) stack. The core engine dynamically resolves the library via `dladdr`, recalculates the valid subset of wrappers for that specific caller, and routes execution perfectly on the fly.





### First-Class Dynamic Dispatch & Runtime Toggling

In addition to automatic fallback, tools can explicitly force a hook into dynamic routing using `audit_hooks::register_dynamic`. This is incredibly powerful for applications that need to toggle hooks on or off mid-execution.

Because `LD_AUDIT` operates in a completely isolated linker namespace (`LM_ID_NEWLM`), the main application cannot directly call the auditor's memory. To solve this, `audit_hook` ships with a safe namespace bridge:

1. Target applications `#include <audit_hook_dynamic.hpp>` and link against the provided `libaudit_hook_dynamic.so` stub.


2. The application calls `ah_set_caller_filter(...)` to toggle its routing.


3. The `LD_AUDIT` engine intercepts the `la_symbind` resolution for the stub library, instantly bridging the call into the auditor's isolated namespace.


4. The Dynamic Dispatcher re-evaluates the active chain, seamlessly changing the behavior of any previously captured function pointers.
