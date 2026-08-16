ACLOCAL_AMFLAGS = -I m4

# Delegate all build targets to the subdirectories
SUBDIRS = src test examples

# Install the public C++ header when running 'make install'
include_HEADERS = include/audit_hook.hpp include/audit_hook_dynamic.h

# ------------------------------------------------------------------------------
# Doxygen Documentation
# ------------------------------------------------------------------------------
if HAVE_DOXYGEN
docs: Doxyfile
	$(DOXYGEN) Doxyfile
else
docs:
	@echo "Doxygen is not installed. Cannot generate documentation."
endif

# Ensure the generated docs directory is cleaned up when running 'make clean'
clean-local:
	-rm -rf docs/
