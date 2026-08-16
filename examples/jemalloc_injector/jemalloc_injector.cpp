#include "audit_hook.hpp"
#include <cstddef>

extern "C" {
    void* je_malloc(size_t size);
    void  je_free(void* ptr);
    void* je_calloc(size_t nmemb, size_t size);
    void* je_realloc(void* ptr, size_t size);
    int   je_posix_memalign(void** memptr, size_t alignment, size_t size);
}

__attribute__((constructor)) void init() {
    audit_hooks::register_replace<je_malloc>("jemalloc_injector", "malloc");
    audit_hooks::register_replace<je_free>("jemalloc_injector", "free");
    audit_hooks::register_replace<je_calloc>("jemalloc_injector", "calloc");
    audit_hooks::register_replace<je_realloc>("jemalloc_injector", "realloc");
    audit_hooks::register_replace<je_posix_memalign>("jemalloc_injector", "posix_memalign");
}