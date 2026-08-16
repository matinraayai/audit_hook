if BUILD_JEMALLOC_EXAMPLE
noinst_LTLIBRARIES = jemalloc_injector.la jemalloc_dynamic_injector.la

# Plugin 1: The Prefixed Static Injector
jemalloc_injector_la_SOURCES = jemalloc_injector.cpp
jemalloc_injector_la_CXXFLAGS = -I$(top_srcdir)/include $(AM_CXXFLAGS)
jemalloc_injector_la_LDFLAGS = -module -avoid-version -shared
jemalloc_injector_la_LIBADD = -ljemalloc

# Plugin 2: The Un-prefixed Dynamic Injector
jemalloc_dynamic_injector_la_SOURCES = jemalloc_dynamic_injector.cpp
jemalloc_dynamic_injector_la_CXXFLAGS = -I$(top_srcdir)/include $(AM_CXXFLAGS)
jemalloc_dynamic_injector_la_LDFLAGS = -module -avoid-version -shared
# Requires dlopen/dlsym
jemalloc_dynamic_injector_la_LIBADD = -ldl
endif