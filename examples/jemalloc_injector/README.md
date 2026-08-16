# jemalloc Injector Examples

This directory demonstrates how to use the `audit_hook` replacement API to forcibly map standard POSIX memory allocation calls (`malloc`, `free`, etc.) directly to `jemalloc` implementations. 

This acts as a safer, highly-performant alternative to the traditional `LD_PRELOAD=libjemalloc.so` technique. 

We provide two distinct approaches depending on whether you are using a custom-prefixed build of `jemalloc` or a standard, un-prefixed package manager build.

---

## The `audit_hook` Advantage

While standard `LD_PRELOAD` forces `jemalloc` onto the entire process space, using `audit_hook` provides three distinct architectural advantages:

1. **Targeted Injection (The Holy Grail):** You can utilize `ah_set_caller_filter` to surgically inject `jemalloc` into your heavy compute libraries, while allowing fragile proprietary third-party libraries to continue safely using `glibc`'s standard allocator.
2. **Zero-Overhead Static Chaining:** Because these plugins use `register_replace` with no caller filters, the dynamic linker natively resolves `malloc` to the target implementations at link-time. There is zero framework overhead in the execution path.
3. **Safe Composability:** If another tool attempts to `register_wrap` the `malloc` function (e.g., a memory leak profiler), `audit_hook` seamlessly treats your injected implementation as the underlying OS function. Multiple tools will cooperate deterministically instead of crashing.

---

## Approach 1: Prefixed Static Injection (`jemalloc_injector.cpp`)

This is the fastest, cleanest approach, but it requires building `jemalloc` from source so that its symbols are exported with a `je_` prefix. This prevents it from immediately overriding `glibc` globally when loaded.

### Building `jemalloc` with the `je_` Prefix
```bash
git clone [https://github.com/jemalloc/jemalloc.git](https://github.com/jemalloc/jemalloc.git)
cd jemalloc
./autogen.sh --with-jemalloc-prefix=je_
make && sudo make install

```

### The Code

Because the functions are prefixed and known at compile time, we simply pass the `jemalloc` function pointers directly into the C++20 registration templates without writing any wrappers:

```cpp
audit_hooks::register_replace<je_malloc>("jemalloc_injector", "malloc");
audit_hooks::register_replace<je_free>("jemalloc_injector", "free");

```

---

## Approach 2: Un-prefixed Dynamic Injection (`jemalloc_dynamic_injector.cpp`)

If you cannot rebuild `jemalloc` (e.g., you must use the standard system package), you can dynamically load the un-prefixed library at runtime.

Because `audit_hook` plugins run in an isolated linker namespace (`LM_ID_NEWLM`), calling `dlopen("libjemalloc.so")` from within the plugin safely isolates the un-prefixed `jemalloc` symbols. They will not accidentally collide with the application's symbols. `audit_hook` becomes the exclusive bridge routing the application's memory requests into your isolated `jemalloc` instance.

### The C++20 Template Constraint & Forwarding Stubs

Because `audit_hook` relies on C++20 Non-Type Template Parameters (NTTPs) for absolute type safety, the function passed to `register_replace` *must* be a compile-time constant. You cannot pass a dynamic runtime pointer returned by `dlsym()`.

To bridge this gap, we create static forwarding stubs:

```cpp
// 1. The runtime pointer loaded via dlsym()
static void* (*dyn_malloc)(size_t) = nullptr;

// 2. The compile-time stub required by the C++20 template
void* injected_malloc(size_t size) { 
    return dyn_malloc(size); 
}

// 3. Registration
audit_hooks::register_replace<injected_malloc>("dynamic_je", "malloc");

```

### Zero-Cost Abstraction via Tail-Call Optimization (TCO)

You might assume that introducing a stub function adds a performance penalty by creating an unnecessary stack frame for every single allocation.

Fortunately, modern C++ compilers recognize that these stubs do nothing but forward their arguments and return. Through **Tail-Call Optimization (TCO)**, the compiler optimizes the stub down to a single assembly jump instruction (e.g., `jmp *dyn_malloc(%rip)`).

At the machine-code level, the stub practically disappears. You gain the strict compile-time type safety of the C++20 templates without sacrificing the raw performance of a direct pointer call!

---

## Running the Examples

If you have `jemalloc` installed on your system, the `audit_hook` build system automatically compiles both examples into `.so` modules. Run them against your target application:

**Run with the Static Prefixed Injector:**

```bash
AH_PLUGINS=./examples/jemalloc_injector/.libs/jemalloc_injector.so \
LD_AUDIT=libaudit_core.so \
./your_target_app

```

**Run with the Dynamic Un-prefixed Injector:**

```bash
AH_PLUGINS=./examples/jemalloc_injector/.libs/jemalloc_dynamic_injector.so \
LD_AUDIT=libaudit_core.so \
./your_target_app

```

```
