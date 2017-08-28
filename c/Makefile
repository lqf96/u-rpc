# Compile flags
CFLAGS = -std=c99 -O2
# Ranlib utility
RANLIB = ranlib

# Library name
LIB_NAME = liburpc.a
# Object files
OBJ_FILES = urpc.o wio-shim.o

# Build library
urpc: $(OBJ_FILES)
	$(AR) rcs $(LIB_NAME) $(OBJ_FILES)
	$(RANLIB) $(LIB_NAME)
# Clean build
clean:
	rm -f $(LIB_NAME) $(OBJ_FILES)
