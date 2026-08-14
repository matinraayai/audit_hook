AM_CPPFLAGS = -I$(top_srcdir)/include

# Define what we are building
check_PROGRAMS = app_simple app_dlsym app_dlopen
check_LTLIBRARIES = libdummy.la libnum.la libnum2.la libtest_replace.la libtest_wrap.la libtest_dlsym_plugin.la libtest_dlopen_plugin.la

# Target Apps
app_simple_SOURCES = app_simple.c

app_dlsym_SOURCES = app_dlsym.c
app_dlsym_LDADD = -ldl

app_dlopen_SOURCES = app_dlopen.c
app_dlopen_LDADD = -ldl

# Dummy & Test Libraries 
libdummy_la_SOURCES = dummy_lib.c
libdummy_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

libnum_la_SOURCES = libnum.c
libnum_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

libnum2_la_SOURCES = libnum2.c
libnum2_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

# Test Plugins
PLUGIN_FLAGS = -fPIC -I$(top_srcdir)/include
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

libtest_dlopen_plugin_la_SOURCES = test_dlopen_plugin.cpp
libtest_dlopen_plugin_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_dlopen_plugin_la_LDFLAGS = $(PLUGIN_LIBS)

# Test execution scripts
TESTS = run_replace.sh run_wrap.sh run_dlsym.sh run_dlopen.sh

# Add to existing check_PROGRAMS and check_LTLIBRARIES
check_PROGRAMS += app_filter
check_LTLIBRARIES += libnum3.la libtest_filter_plugin.la

# Target App
app_filter_SOURCES = app_filter.c
app_filter_LDADD = libnum3.la

# Target Library
libnum3_la_SOURCES = libnum3.c
libnum3_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

# Test Plugin
libtest_filter_plugin_la_SOURCES = test_filter_plugin.cpp
libtest_filter_plugin_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_filter_plugin_la_LDFLAGS = $(PLUGIN_LIBS)

# Add run_filter.sh to the existing TESTS array
TESTS += run_filter.sh

# --- Composition Test ---
check_PROGRAMS += app_comp
check_LTLIBRARIES += libcomp.la libtest_comp_plugin.la

app_comp_SOURCES = app_comp.c
app_comp_LDADD = libcomp.la

libcomp_la_SOURCES = libcomp.c
libcomp_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

libtest_comp_plugin_la_SOURCES = test_comp_plugin.cpp
libtest_comp_plugin_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_comp_plugin_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_comp.sh

# --- Filter State Test ---
check_PROGRAMS += app_filter_state
check_LTLIBRARIES += libtest_filter_state_plugin.la

app_filter_state_SOURCES = app_filter_state.c

libtest_filter_state_plugin_la_SOURCES = test_filter_state_plugin.cpp
libtest_filter_state_plugin_la_CXXFLAGS = $(PLUGIN_FLAGS)
libtest_filter_state_plugin_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_filter_state.sh

# --- Shared Libraries for Dynamic Dispatch Tests ---
check_LTLIBRARIES += libdispatch_target.la libdispatch_x.la libdispatch_y.la

libdispatch_target_la_SOURCES = libdispatch_target.c
libdispatch_target_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

libdispatch_x_la_SOURCES = libdispatch_x.c
libdispatch_x_la_LIBADD = libdispatch_target.la
libdispatch_x_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

libdispatch_y_la_SOURCES = libdispatch_y.c
libdispatch_y_la_LIBADD = libdispatch_target.la
libdispatch_y_la_LDFLAGS = -shared -avoid-version -rpath $(abs_builddir)

# --- Test 1: Divergent Wrapper Chain ---
check_PROGRAMS += app_test_divergent
check_LTLIBRARIES += libplugin_div_a.la libplugin_div_b.la

app_test_divergent_SOURCES = app_test_divergent.c
app_test_divergent_LDADD = libdispatch_x.la libdispatch_y.la

libplugin_div_a_la_SOURCES = plugin_div_a.cpp
libplugin_div_a_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_div_a_la_LDFLAGS = $(PLUGIN_LIBS)

libplugin_div_b_la_SOURCES = plugin_div_b.cpp
libplugin_div_b_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_div_b_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_test_divergent.sh

# --- Test 2: Pure Static Chain ---
check_PROGRAMS += app_test_static
check_LTLIBRARIES += libplugin_stat_a.la libplugin_stat_b.la

app_test_static_SOURCES = app_test_static.c
app_test_static_LDADD = libdispatch_x.la libdispatch_y.la

libplugin_stat_a_la_SOURCES = plugin_stat_a.cpp
libplugin_stat_a_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_stat_a_la_LDFLAGS = $(PLUGIN_LIBS)

libplugin_stat_b_la_SOURCES = plugin_stat_b.cpp
libplugin_stat_b_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_stat_b_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_test_static.sh

# --- Test 3: Mixed Divergence ---
check_PROGRAMS += app_test_mixed
check_LTLIBRARIES += libplugin_mix_a.la libplugin_mix_b.la

app_test_mixed_SOURCES = app_test_mixed.c
app_test_mixed_LDADD = libdispatch_x.la libdispatch_y.la

libplugin_mix_a_la_SOURCES = plugin_mix_a.cpp
libplugin_mix_a_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_mix_a_la_LDFLAGS = $(PLUGIN_LIBS)

libplugin_mix_b_la_SOURCES = plugin_mix_b.cpp
libplugin_mix_b_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_mix_b_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_test_mixed.sh

# --- Test 4: Dynamic Dispatch Runtime Toggle ---
check_PROGRAMS += app_fp_dynamic
check_LTLIBRARIES += libplugin_fp_dynamic.la

app_fp_dynamic_SOURCES = app_fp_dynamic.c
# Link against the target library and the new dummy library
app_fp_dynamic_LDADD = libdispatch_target.la ../src/libaudit_hook_dynamic.la

libplugin_fp_dynamic_la_SOURCES = plugin_fp_dynamic.cpp
libplugin_fp_dynamic_la_CXXFLAGS = $(PLUGIN_FLAGS)
libplugin_fp_dynamic_la_LDFLAGS = $(PLUGIN_LIBS)

TESTS += run_fp_dynamic.sh

