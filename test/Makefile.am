# Ensure the compiler can find audit_hook.hpp and audit_hook_dynamic.h
AM_CPPFLAGS = -I$(top_srcdir)/include

# ------------------------------------------------------------------------------
# Shared Libraries & Plugins
# ------------------------------------------------------------------------------
lib_LTLIBRARIES = \
    libnum.la \
    libnum2.la \
    libnum3.la \
    libcomp.la \
    libdummy.la \
    libdispatch_x.la \
    libdispatch_y.la \
    libdispatch_target.la \
    libhammer_math.la \
    libhammer_check.la \
    libtest_replace.la \
    libtest_wrap.la \
    libtest_dlsym_plugin.la \
    libtest_dlopen_plugin.la \
    libtest_filter_plugin.la \
    libtest_comp_plugin.la \
    libtest_filter_state_plugin.la \
    libplugin_stat_a.la \
    libplugin_stat_b.la \
    libplugin_div_a.la \
    libplugin_div_b.la \
    libplugin_mix_a.la \
    libplugin_mix_b.la \
    libplugin_fp_dynamic.la \
    libplugin_hammer.la

# Target & Dummy C Libraries
libnum_la_SOURCES = libnum.c
libnum2_la_SOURCES = libnum2.c
libnum3_la_SOURCES = libnum3.c
libcomp_la_SOURCES = libcomp.c
libdummy_la_SOURCES = dummy_lib.c
libdispatch_x_la_SOURCES = libdispatch_x.c
libdispatch_y_la_SOURCES = libdispatch_y.c
libdispatch_target_la_SOURCES = libdispatch_target.c

# Hammer test architecture
libhammer_math_la_SOURCES = libhammer_math.cpp
libhammer_math_la_CXXFLAGS = $(AM_CXXFLAGS)

libhammer_check_la_SOURCES = libhammer_check.cpp
libhammer_check_la_CXXFLAGS = $(AM_CXXFLAGS)
libhammer_check_la_LIBADD = libhammer_math.la

# Plugins
libtest_replace_la_SOURCES = test_replace.cpp
libtest_wrap_la_SOURCES = test_wrap.cpp
libtest_dlsym_plugin_la_SOURCES = test_dlsym_plugin.cpp
libtest_dlopen_plugin_la_SOURCES = test_dlopen_plugin.cpp
libtest_filter_plugin_la_SOURCES = test_filter_plugin.cpp
libtest_comp_plugin_la_SOURCES = test_comp_plugin.cpp
libtest_filter_state_plugin_la_SOURCES = test_filter_state_plugin.cpp
libplugin_stat_a_la_SOURCES = plugin_stat_a.cpp
libplugin_stat_b_la_SOURCES = plugin_stat_b.cpp
libplugin_div_a_la_SOURCES = plugin_div_a.cpp
libplugin_div_b_la_SOURCES = plugin_div_b.cpp
libplugin_mix_a_la_SOURCES = plugin_mix_a.cpp
libplugin_mix_b_la_SOURCES = plugin_mix_b.cpp
libplugin_fp_dynamic_la_SOURCES = plugin_fp_dynamic.cpp
libplugin_hammer_la_SOURCES = plugin_hammer.cpp
libplugin_hammer_la_CXXFLAGS = $(AM_CXXFLAGS)

# ------------------------------------------------------------------------------
# Test Executables
# ------------------------------------------------------------------------------
check_PROGRAMS = \
    app_simple \
    app_dlsym \
    app_dlopen \
    app_filter \
    app_comp \
    app_filter_state \
    app_test_static \
    app_test_divergent \
    app_test_mixed \
    app_fp_dynamic \
    app_hammer

app_simple_SOURCES = app_simple.c
app_simple_LDADD = libnum.la libnum2.la

app_dlsym_SOURCES = app_dlsym.c
app_dlsym_LDADD = libnum.la
app_dlsym_LDFLAGS = -ldl

app_dlopen_SOURCES = app_dlopen.c
app_dlopen_LDADD = libnum.la
app_dlopen_LDFLAGS = -ldl

app_filter_SOURCES = app_filter.c
app_filter_LDADD = libnum.la libnum2.la libnum3.la

app_comp_SOURCES = app_comp.c
app_comp_LDADD = libcomp.la

app_filter_state_SOURCES = app_filter_state.c
app_filter_state_LDADD = libnum.la libnum2.la libnum3.la

app_test_static_SOURCES = app_test_static.c
app_test_static_LDADD = libdispatch_x.la libdispatch_y.la libdispatch_target.la

app_test_divergent_SOURCES = app_test_divergent.c
app_test_divergent_LDADD = libdispatch_x.la libdispatch_y.la libdispatch_target.la

app_test_mixed_SOURCES = app_test_mixed.c
app_test_mixed_LDADD = libdispatch_x.la libdispatch_y.la libdispatch_target.la

app_fp_dynamic_SOURCES = app_fp_dynamic.c
app_fp_dynamic_LDADD = libdispatch_target.la $(top_builddir)/src/libaudit_hook_dynamic.la
app_fp_dynamic_LDFLAGS = -ldl

app_hammer_SOURCES = app_hammer.cpp
app_hammer_LDFLAGS = -ldl

# ------------------------------------------------------------------------------
# Test Scripts & Distribution
# ------------------------------------------------------------------------------
TESTS = \
    run_replace.sh \
    run_wrap.sh \
    run_dlsym.sh \
    run_dlopen.sh \
    run_filter.sh \
    run_comp.sh \
    run_filter_state.sh \
    run_test_static.sh \
    run_test_divergent.sh \
    run_test_mixed.sh \
    run_fp_dynamic.sh \
    run_hammer.sh

EXTRA_DIST = $(TESTS) hammer_math.hpp