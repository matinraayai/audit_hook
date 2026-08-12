# Define what we are building
check_PROGRAMS = app_simple app_dlsym
check_LTLIBRARIES = libdummy.la libtest_replace.la libtest_wrap.la libtest_dlsym_plugin.la

# Target Apps
app_simple_SOURCES = app_simple.c

app_dlsym_SOURCES = app_dlsym.c
app_dlsym_LDADD = -ldl

# Dummy Library 
# FIX: Added -rpath to force Libtool to generate the .so file!
libdummy_la_SOURCES = dummy_lib.c
libdummy_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

# Test Plugins
PLUGIN_FLAGS = -fPIC -I$(top_srcdir)/include

# FIX: Added -rpath to force Libtool to generate the .so files!
PLUGIN_LIBS = -module -shared -avoid-version -rpath $(abs_builddir) $(top_builddir)/src/libaudit_core.la

libtest_replace_la_SOURCES = test_replace.cpp
libtest_replace_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_replace_la_LDFLAGS = $(PLUGIN_LIBS)

libtest_wrap_la_SOURCES = test_wrap.cpp
libtest_wrap_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_wrap_la_LDFLAGS = $(PLUGIN_LIBS)

libtest_dlsym_plugin_la_SOURCES = test_dlsym_plugin.cpp
libtest_dlsym_plugin_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_dlsym_plugin_la_LDFLAGS = $(PLUGIN_LIBS)

# Test execution scripts
TESTS = run_replace.sh run_wrap.sh run_dlsym.sh