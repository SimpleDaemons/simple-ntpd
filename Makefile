# Simple NTP Daemon Makefile
# Provides convenient build targets and deployment options
# Copyright 2024 SimpleDaemons
# Licensed under the Apache License, Version 2.0

# Project configuration
PROJECT_NAME = simple-ntpd
VERSION = 0.1.0
BUILD_DIR = build
INSTALL_DIR = /usr/local
CONFIG_DIR = /etc/simple-ntpd
LOG_DIR = /var/log
DATA_DIR = /var/lib/simple-ntpd

# Build options
CMAKE_OPTS = -DCMAKE_BUILD_TYPE=Release
CMAKE_OPTS += -DBUILD_TESTS=ON
CMAKE_OPTS += -DBUILD_EXAMPLES=OFF
CMAKE_OPTS += -DENABLE_LOGGING=ON
CMAKE_OPTS += -DENABLE_IPV6=ON

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Default target
.PHONY: all
all: build

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Configure with CMake
.PHONY: configure
configure: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_OPTS) ..

# Build the project
.PHONY: build
build: configure
	$(MAKE) -C $(BUILD_DIR)

# Build with debug information
.PHONY: debug
debug: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Debug
debug: build

# Build with optimization
.PHONY: release
release: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Release
release: build

# Build with sanitizers
.PHONY: sanitize
sanitize: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Debug
sanitize: CMAKE_OPTS += -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
sanitize: build

# Clean build directory
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Clean and rebuild
.PHONY: rebuild
rebuild: clean build

# Run tests
.PHONY: test
test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

# Run tests with verbose output
.PHONY: test-verbose
test-verbose: build
	cd $(BUILD_DIR) && ctest --output-on-failure -V

# Install the application
.PHONY: install
install: build
	cd $(BUILD_DIR) && $(MAKE) install

# Install to custom directory
.PHONY: install-custom
install-custom: build
	cd $(BUILD_DIR) && $(MAKE) DESTDIR=$(DESTDIR) install

# Uninstall the application
.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_DIR)/bin/$(PROJECT_NAME)
	rm -f $(INSTALL_DIR)/lib/lib$(PROJECT_NAME)-core.a
	rm -f $(INSTALL_DIR)/lib/cmake/$(PROJECT_NAME)/*.cmake
	rm -rf $(CONFIG_DIR)
	rm -rf $(LOG_DIR)
	rm -rf $(DATA_DIR)

# Create package
.PHONY: package
package: build
	cd $(BUILD_DIR) && $(MAKE) package

# Create source package
.PHONY: package-source
package-source: build
	cd $(BUILD_DIR) && $(MAKE) package_source

# Build for specific platform
.PHONY: build-macos
build-macos: CMAKE_OPTS += -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
build-macos: build

.PHONY: build-linux
build-linux: build

.PHONY: build-windows
build-windows: build

# Development targets
.PHONY: format
format:
	find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

.PHONY: lint
lint:
	cppcheck --enable=all --std=c++17 src include

.PHONY: check
check: format lint test

# Documentation targets
.PHONY: docs
docs:
	doxygen Doxyfile

.PHONY: docs-clean
docs-clean:
	rm -rf docs/html docs/latex

# Deployment targets
.PHONY: deploy-prep
deploy-prep:
	sudo mkdir -p $(CONFIG_DIR)
	sudo mkdir -p $(LOG_DIR)
	sudo mkdir -p $(DATA_DIR)
	sudo chown -R root:root $(CONFIG_DIR)
	sudo chmod 755 $(CONFIG_DIR)
	sudo chmod 755 $(LOG_DIR)
	sudo chmod 755 $(DATA_DIR)

.PHONY: deploy-config
deploy-config: deploy-prep
	sudo cp config/simple-ntpd.conf.example $(CONFIG_DIR)/simple-ntpd.conf
	sudo chmod 644 $(CONFIG_DIR)/simple-ntpd.conf

.PHONY: deploy-systemd
deploy-systemd: deploy-prep
	sudo cp deployment/systemd/simple-ntpd.service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable simple-ntpd

.PHONY: deploy-init
deploy-init: deploy-prep
	sudo cp deployment/init.d/simple-ntpd /etc/init.d/
	sudo chmod 755 /etc/init.d/simple-ntpd
	sudo update-rc.d simple-ntpd defaults

# Service management targets
.PHONY: start
start:
	sudo systemctl start simple-ntpd

.PHONY: stop
stop:
	sudo systemctl stop simple-ntpd

.PHONY: restart
restart:
	sudo systemctl restart simple-ntpd

.PHONY: status
status:
	sudo systemctl status simple-ntpd

# Docker targets
.PHONY: docker-build
docker-build:
	docker build -t $(PROJECT_NAME):$(VERSION) .

.PHONY: docker-run
docker-run:
	docker run -d --name $(PROJECT_NAME) -p 123:123/udp $(PROJECT_NAME):$(VERSION)

.PHONY: docker-stop
docker-stop:
	docker stop $(PROJECT_NAME)
	docker rm $(PROJECT_NAME)

# Help target
.PHONY: help
help:
	@echo "Simple NTP Daemon Build System"
	@echo "=============================="
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build the project"
	@echo "  debug            - Build with debug information"
	@echo "  release          - Build with optimization"
	@echo "  sanitize         - Build with sanitizers"
	@echo "  clean            - Clean build directory"
	@echo "  rebuild          - Clean and rebuild"
	@echo "  test             - Run tests"
	@echo "  test-verbose     - Run tests with verbose output"
	@echo "  install          - Install the application"
	@echo "  uninstall        - Uninstall the application"
	@echo "  package          - Create package"
	@echo "  package-source   - Create source package"
	@echo "  format           - Format source code"
	@echo "  lint             - Run static analysis"
	@echo "  check            - Format, lint, and test"
	@echo "  docs             - Generate documentation"
	@echo "  deploy-prep      - Prepare deployment directories"
	@echo "  deploy-config    - Deploy configuration files"
	@echo "  deploy-systemd   - Deploy systemd service"
	@echo "  deploy-init      - Deploy init.d script"
	@echo "  start            - Start the service"
	@echo "  stop             - Stop the service"
	@echo "  restart          - Restart the service"
	@echo "  status           - Show service status"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Platform-specific targets:"
	@echo "  build-macos      - Build for macOS"
	@echo "  build-linux      - Build for Linux"
	@echo "  build-windows    - Build for Windows"
	@echo ""
	@echo "Examples:"
	@echo "  make debug       - Build with debug information"
	@echo "  make test        - Build and run tests"
	@echo "  make install     - Build and install"
	@echo "  make deploy-config - Deploy configuration files"

# Default target
.DEFAULT_GOAL := all
