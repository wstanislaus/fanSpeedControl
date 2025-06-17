# Default build directory
BUILD_DIR ?= build

# Default install directory
INSTALL_DIR ?= /usr/local

# Default target
.PHONY: all
all: $(BUILD_DIR)
	@echo "Building project..."
	@cmake --build $(BUILD_DIR) -- -j$(shell nproc)

# Create build directory and run CMake
$(BUILD_DIR):
	@echo "Creating build directory..."
	@mkdir -p $(BUILD_DIR)
	@echo "Running CMake..."
	@cd $(BUILD_DIR) && cmake ..

# Install the project
.PHONY: install
install: all
	@echo "Installing project..."
	@cmake --install $(BUILD_DIR) --prefix $(INSTALL_DIR)

# Uninstall the project
.PHONY: uninstall
uninstall:
	@echo "Uninstalling project..."
	@xargs rm -f < $(BUILD_DIR)/install_manifest.txt

# Clean build files
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)

# Show help message
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  install    - Install the project to $(INSTALL_DIR)"
	@echo "  uninstall  - Uninstall the project"
	@echo "  clean      - Remove build files"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_DIR    - Build directory (default: build)"
	@echo "  INSTALL_DIR  - Install directory (default: /usr/local)" 