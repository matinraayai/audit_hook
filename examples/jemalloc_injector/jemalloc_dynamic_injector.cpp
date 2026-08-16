#include "audit_hook.hpp"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>

// -----------------------------------------------------------------------------
// 1. Function pointers to hold the dynamically resolved jemalloc addresses
// -----------------------------------------------------------------------------
static void* (*dyn_malloc)(size_t) = nullptr;
static void  (*dyn_free)(void*) = nullptr;
static void* (*dyn_calloc)(size_t, size_t) = nullptr;
static void* (*dyn_realloc)(void*, size_t) = nullptr;

// -----------------------------------------------------------------------------
// 2. Static wrapper functions to satisfy C++20 Compile-Time Templates
// -----------------------------------------------------------------------------
void* injected_malloc(size_t size) { 
    return dyn_malloc(size); 
}

void injected_free(void* ptr) { 
    dyn_free(ptr); 
}

void* injected_calloc(size_t n, size_t size) { 
    return dyn_calloc(n, size); 
}

void* injected_realloc(void* ptr, size_t size) { 
    return dyn_realloc(ptr, size); 
}

// -----------------------------------------------------------------------------
// 3. Initialization: Load, Link, and Replace
// -----------------------------------------------------------------------------
__attribute__((constructor)) void init() {
    // Load standard, un-prefixed jemalloc into the auditor's isolated namespace (LM_ID_NEWLM)
    // RTLD_LOCAL ensures its un-prefixed symbols don't bleed into other plugins.
    void* handle = dlopen("libjemalloc.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "[DynamicJe] Failed to load libjemalloc.so: %s\n", dlerror());
        return;
    }

    // Resolve the standard un-prefixed symbols from the loaded library
    dyn_malloc  = reinterpret_cast<void* (*)(size_t)>(dlsym(handle, "malloc"));
    dyn_free    = reinterpret_cast<void  (*)(void*)>(dlsym(handle, "free"));
    dyn_calloc  = reinterpret_cast<void* (*)(size_t, size_t)>(dlsym(handle, "calloc"));
    dyn_realloc = reinterpret_cast<void* (*)(void*, size_t)>(dlsym(handle, "realloc"));

    if (!dyn_malloc || !dyn_free || !dyn_calloc || !dyn_realloc) {
        fprintf(stderr, "[DynamicJe] Failed to resolve jemalloc symbols. Is this the correct library?\n");
        return;
    }

    // Register our static wrappers to replace the application's OS calls.
    audit_hooks::register_replace<injected_malloc>("dynamic_je", "malloc");
    audit_hooks::register_replace<injected_free>("dynamic_je", "free");
    audit_hooks::register_replace<injected_calloc>("dynamic_je", "calloc");
    audit_hooks::register_replace<injected_realloc>("dynamic_je", "realloc");
}