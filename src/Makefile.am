# Define the core LD_AUDIT engine library
lib_LTLIBRARIES = libaudit_core.la

# Sources and compiler flags
libaudit_core_la_SOURCES = audit_core.cpp
libaudit_core_la_CXXFLAGS = -fPIC -I$(top_srcdir)/include

# Linker flags: create a shared library, link libdl, and avoid versioning numbers (.so.1.0.0)
libaudit_core_la_LDFLAGS = -shared -ldl -avoid-version