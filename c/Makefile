# Compile flags
CFLAGS = -std=c99 -O2
# Ranlib utility
RANLIB = ranlib

# Library name
LIB_NAME = liburpc.a
# Object files
OBJ_FILES = urpc.o

# Use WIO
ifdef WIO_PATH
	CFLAGS := $(CFLAGS) -I$(WIO_PATH)
else
	OBJ_FILES := $(OBJ_FILES) wio-shim.o
endif

# Build library
urpc: $(OBJ_FILES)
	$(AR) rcs $(LIB_NAME) $(OBJ_FILES)
	$(RANLIB) $(LIB_NAME)
# Clean build
clean:
	rm -f $(LIB_NAME) $(OBJ_FILES)
