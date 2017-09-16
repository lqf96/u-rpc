# Compile flags
CFLAGS = -std=c99 -O2
# Ranlib utility
RANLIB = ranlib

# Dynamic library configuration
ifeq ($(shell uname -s), Darwin)
	LDFLAGS := $(LDFLAGS) -dynamiclib
	DYLIB_SUFFIX = .dylib
else
	CFLAGS := $(CFLAGS) -fPIC
	LDFLAGS := $(LDFLAGS) -shared
	DYLIB_SUFFIX = .so
endif

# Library name
LIB_NAME = liburpc
# Object files
OBJ_FILES = urpc.o wio-shim.o

# Static library
static: $(OBJ_FILES)
	$(AR) rcs $(LIB_NAME).a $(OBJ_FILES)
	$(RANLIB) $(LIB_NAME).a
# Dynamic library
dynamic: $(OBJ_FILES)
	$(CC) -o $(LIB_NAME)$(DYLIB_SUFFIX) $(OBJ_FILES) $(LDFLAGS)

# Clean build
clean:
	rm -f $(LIB_NAME).* $(OBJ_FILES)
