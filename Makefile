# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -fPIC
DEBUG_FLAGS = -g -O0 -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS
    LIB_EXT = dylib
    LIB_SONAME = lib$(LIB_NAME).$(LIB_VERSION).$(LIB_EXT)
    LIB_FULL = lib$(LIB_NAME).$(LIB_VERSION).$(LIB_EXT)
    LIB_SHORT = lib$(LIB_NAME).$(LIB_EXT)
    SHARED_FLAGS = -dynamiclib -install_name $(LIB_DIR)/$(LIB_FULL)
else
    # Linux
    LIB_EXT = so
    LIB_SONAME = lib$(LIB_NAME).$(LIB_EXT).$(LIB_VERSION)
    LIB_FULL = lib$(LIB_NAME).$(LIB_EXT).$(LIB_VERSION)
    LIB_SHORT = lib$(LIB_NAME).$(LIB_EXT)
    SHARED_FLAGS = -shared -Wl,-soname,$(LIB_SONAME)
endif

# Directories
SRC_DIR = lib
INCLUDE_DIR = include
BUILD_DIR = build
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin
EXAMPLE_DIR = examples

# Library name and version
LIB_NAME = kuserspace
LIB_VERSION = 1.0.0

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

# Example files
EXAMPLES = $(wildcard $(EXAMPLE_DIR)/*.cpp)
EXAMPLE_BINS = $(EXAMPLES:$(EXAMPLE_DIR)/%.cpp=$(BIN_DIR)/%)

# Default target
all: release

# Create necessary directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(LIB_DIR)
	mkdir -p $(BIN_DIR)

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(BUILD_DIR) $(LIB_DIR)/$(LIB_FULL) $(EXAMPLE_BINS)

# Release build
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(BUILD_DIR) $(LIB_DIR)/$(LIB_FULL) $(EXAMPLE_BINS)

# Build the shared library
$(LIB_DIR)/$(LIB_FULL): $(OBJS)
	$(CXX) $(SHARED_FLAGS) -o $@ $^
	ln -sf $(LIB_FULL) $(LIB_DIR)/$(LIB_SHORT)

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

# Build examples
$(BIN_DIR)/%: $(EXAMPLE_DIR)/%.cpp $(LIB_DIR)/$(LIB_FULL)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -L$(LIB_DIR) -l$(LIB_NAME) $< -o $@

# Install targets
install: release
	install -d $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/include/$(LIB_NAME)
	install -m 644 $(LIB_DIR)/$(LIB_FULL) $(DESTDIR)/usr/lib/
	ln -sf $(LIB_FULL) $(DESTDIR)/usr/lib/$(LIB_SHORT)
	install -m 644 $(INCLUDE_DIR)/*.h $(DESTDIR)/usr/include/$(LIB_NAME)/

# Uninstall targets
uninstall:
	rm -f $(DESTDIR)/usr/lib/$(LIB_FULL)
	rm -f $(DESTDIR)/usr/lib/$(LIB_SHORT)
	rm -rf $(DESTDIR)/usr/include/$(LIB_NAME)

# Clean targets
clean:
	rm -rf $(BUILD_DIR)

# Include dependency files
-include $(DEPS)

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build release version (default)"
	@echo "  debug      - Build debug version"
	@echo "  release    - Build release version"
	@echo "  install    - Install library and headers"
	@echo "  uninstall  - Remove installed files"
	@echo "  clean      - Remove build files"
	@echo "  help       - Show this help message"

# Phony targets
.PHONY: all debug release install uninstall clean help 