AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LTLIBRARIES = libaudit_core.la libaudit_hook_dynamic.la

# The LD_AUDIT backend engine
libaudit_core_la_SOURCES = audit_core.cpp
libaudit_core_la_CXXFLAGS = -fPIC -shared
libaudit_core_la_LDFLAGS = -module -avoid-version

# The dummy library for application linkage
libaudit_hook_dynamic_la_SOURCES = audit_hook_dynamic.cpp
libaudit_hook_dynamic_la_CXXFLAGS = -fPIC
libaudit_hook_dynamic_la_LDFLAGS = -shared -avoid-version