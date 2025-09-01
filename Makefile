# Makefile for simple-ntpd
# Simple NTP Daemon - A lightweight and secure NTP server
# Copyright 2024 SimpleDaemons <info@simpledaemons.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Variables
PROJECT_NAME = simple-ntpd
VERSION = 0.1.0
BUILD_DIR = build
DIST_DIR = dist
PACKAGE_DIR = packaging

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    # Windows specific settings
    CXX = cl
    CXXFLAGS = /std:c++17 /W3 /O2 /DNDEBUG /EHsc
    LDFLAGS = ws2_32.lib crypt32.lib
    # Windows specific flags
    CXXFLAGS += /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS
    # Detect processor cores for parallel builds
    PARALLEL_JOBS = $(shell echo %NUMBER_OF_PROCESSORS%)
    # Windows install paths
    INSTALL_PREFIX = C:/Program Files/$(PROJECT_NAME)
    CONFIG_DIR = $(INSTALL_PREFIX)/etc
    # Windows file extensions
    EXE_EXT = .exe
    LIB_EXT = .lib
    DLL_EXT = .dll
    # Windows commands
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    CP = copy
    # Check if running in Git Bash or similar
    ifeq ($(shell echo $$SHELL),/usr/bin/bash)
        # Running in Git Bash, use Unix commands
        RM = rm -rf
        RMDIR = rm -rf
        MKDIR = mkdir -p
        CP = cp -r
        # Use Unix-style paths
        INSTALL_PREFIX = /c/Program\ Files/$(PROJECT_NAME)
        CONFIG_DIR = /c/Program\ Files/$(PROJECT_NAME)/etc
    endif
else ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    CXX = clang++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto
    # macOS specific flags
    CXXFLAGS += -target x86_64-apple-macos12.0 -target arm64-apple-macos12.0
    # Detect processor cores for parallel builds
    PARALLEL_JOBS = $(shell sysctl -n hw.ncpu)
    # macOS install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = $(INSTALL_PREFIX)/etc/$(PROJECT_NAME)
    # Unix file extensions
    EXE_EXT =
    LIB_EXT = .dylib
    DLL_EXT = .dylib
    # Unix commands
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
else
    PLATFORM = linux
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG
    LDFLAGS = -lssl -lcrypto -lpthread
    # Linux specific flags
    PARALLEL_JOBS = $(shell nproc)
    # Linux install paths
    INSTALL_PREFIX = /usr/local
    CONFIG_DIR = /etc/$(PROJECT_NAME)
    # Unix file extensions
    EXE_EXT =
    LIB_EXT = .so
    DLL_EXT = .so
    # Unix commands
    RM = rm -rf
    RMDIR = rm -rf
    MKDIR = mkdir -p
    CP = cp -r
endif

# Directories
SRC_DIR = src
INCLUDE_DIR = include
CONFIG_DIR_SRC = config
SCRIPTS_DIR = scripts
DEPLOYMENT_DIR = deployment

# Legacy variables for compatibility
INSTALL_DIR = $(INSTALL_PREFIX)
LOG_DIR = /var/log
DATA_DIR = /var/lib/$(PROJECT_NAME)

# Default target
all: build

# Create build directory
$(BUILD_DIR)-dir:
ifeq ($(PLATFORM),windows)
	$(MKDIR) $(BUILD_DIR)
else
	$(MKDIR) $(BUILD_DIR)
endif

# Build using CMake
build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 && cmake --build . --config Release
else
	cd $(BUILD_DIR) && cmake .. && make -j$(PARALLEL_JOBS)
endif

# Clean build
clean:
ifeq ($(PLATFORM),windows)
	$(RMDIR) $(BUILD_DIR)
	$(RMDIR) $(DIST_DIR)
else
	$(RM) $(BUILD_DIR)
	$(RM) $(DIST_DIR)
endif

# Install
install: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake --install . --prefix "$(INSTALL_PREFIX)"
else
	cd $(BUILD_DIR) && sudo make install
endif

# Uninstall
uninstall:
ifeq ($(PLATFORM),windows)
	$(RMDIR) "$(INSTALL_PREFIX)"
else
	sudo rm -f $(INSTALL_PREFIX)/bin/$(PROJECT_NAME)
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).so
	sudo rm -f $(INSTALL_PREFIX)/lib/lib$(PROJECT_NAME).dylib
	sudo rm -rf $(INSTALL_PREFIX)/include/$(PROJECT_NAME)
	sudo rm -rf $(CONFIG_DIR)
endif

# Test
test: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && make test
endif

# Generic package target (platform-specific)
package: build
ifeq ($(PLATFORM),macos)
	@echo "Building macOS packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DragNDrop
	cd $(BUILD_DIR) && cpack -G productbuild
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.dmg $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.pkg $(DIST_DIR)/ 2>/dev/null || true
	@echo "macOS packages created: DMG and PKG"
else ifeq ($(PLATFORM),linux)
	@echo "Building Linux packages..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G RPM
	cd $(BUILD_DIR) && cpack -G DEB
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.rpm $(DIST_DIR)/ 2>/dev/null || true
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.deb $(DIST_DIR)/ 2>/dev/null || true
	@echo "Linux packages created: RPM and DEB"
else ifeq ($(PLATFORM),windows)
	@echo "Building Windows packages..."
	@$(MKDIR) $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G WIX
	cd $(BUILD_DIR) && cpack -G ZIP
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.msi $(DIST_DIR)/ 2>/dev/null || true
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.zip $(DIST_DIR)/ 2>/dev/null || true
	@echo "Windows packages created: MSI and ZIP"
else
	@echo "Package generation not supported on this platform"
endif

# Package RPM (Linux only)
package-rpm: build
ifeq ($(PLATFORM),linux)
	@echo "Building RPM package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G RPM
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.rpm $(DIST_DIR)/
else
	@echo "RPM packages are only supported on Linux"
endif

# Package DEB (Linux only)
package-deb: build
ifeq ($(PLATFORM),linux)
	@echo "Building DEB package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DEB
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.deb $(DIST_DIR)/
else
	@echo "DEB packages are only supported on Linux"
endif

# Package DMG (macOS only)
package-dmg: build
ifeq ($(PLATFORM),macos)
	@echo "Building DMG package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G DragNDrop
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.dmg $(DIST_DIR)/
else
	@echo "DMG packages are only supported on macOS"
endif

# Package PKG (macOS only)
package-pkg: build
ifeq ($(PLATFORM),macos)
	@echo "Building PKG package..."
	@mkdir -p $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G productbuild
	mv $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.pkg $(DIST_DIR)/
else
	@echo "PKG packages are only supported on macOS"
endif

# Package MSI (Windows only)
package-msi: build
ifeq ($(PLATFORM),windows)
	@echo "Building MSI package..."
	@$(MKDIR) $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G WIX
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.msi $(DIST_DIR)/
else
	@echo "MSI packages are only supported on Windows"
endif

# Package ZIP (Windows only)
package-zip: build
ifeq ($(PLATFORM),windows)
	@echo "Building ZIP package..."
	@$(MKDIR) $(DIST_DIR)
	cd $(BUILD_DIR) && cpack -G ZIP
	$(CP) $(BUILD_DIR)/$(PROJECT_NAME)-$(VERSION)-*.zip $(DIST_DIR)/
else
	@echo "ZIP packages are only supported on Windows"
endif

# Package using build scripts (recommended)
package-script:
ifeq ($(PLATFORM),macos)
	@echo "Building macOS package with build script..."
	./scripts/build-macos.sh --package
else ifeq ($(PLATFORM),linux)
	@echo "Building Linux package with build script..."
	./scripts/build-linux.sh --package
else ifeq ($(PLATFORM),windows)
	@echo "Building Windows package with build script..."
	scripts\build-windows.bat --package
else
	@echo "Package generation not supported on this platform"
endif

# Package source code
package-source: build
	@echo "Creating source package..."
	@mkdir -p $(DIST_DIR)
ifeq ($(PLATFORM),windows)
	powershell -Command "Compress-Archive -Path '$(SRC_DIR)', '$(INCLUDE_DIR)', 'CMakeLists.txt', 'Makefile', 'README.md', 'LICENSE' -DestinationPath '$(DIST_DIR)\$(PROJECT_NAME)-$(VERSION)-src.zip' -Force"
else
	tar -czf $(DIST_DIR)/$(PROJECT_NAME)-$(VERSION)-src.tar.gz \
		$(SRC_DIR) \
		$(INCLUDE_DIR) \
		CMakeLists.txt \
		Makefile \
		README.md \
		LICENSE
endif
	@echo "Source package created"

# Package all formats
package-all:
ifeq ($(PLATFORM),macos)
	package-dmg package-pkg package-script
else ifeq ($(PLATFORM),windows)
	package-msi package-zip package-script
else
	package-rpm package-deb package-script
endif

# Development targets
dev-build: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug && cmake --build . --config Debug
else
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$(PARALLEL_JOBS)
endif

dev-test: dev-build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && make test
endif

# Documentation
docs: build
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake --build . --target docs
else
	cd $(BUILD_DIR) && make docs
endif

# Static analysis
analyze: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug -DENABLE_ANALYZE=ON && cmake --build . --config Debug
else
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ANALYZE=ON && make -j$(PARALLEL_JOBS)
endif

# Coverage
coverage: $(BUILD_DIR)-dir
ifeq ($(PLATFORM),windows)
	@echo "Coverage reporting not yet implemented for Windows"
	cd $(BUILD_DIR) && cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON && cmake --build . --config Debug
	cd $(BUILD_DIR) && ctest --output-on-failure
else
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON && make -j$(PARALLEL_JOBS)
	cd $(BUILD_DIR) && make test
	cd $(BUILD_DIR) && lcov --capture --directory . --output-file coverage.info
	cd $(BUILD_DIR) && genhtml coverage.info --output-directory coverage_report
endif

# Format code
format:
ifeq ($(PLATFORM),windows)
	@echo "Code formatting on Windows requires clang-format to be installed"
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i; \
	else \
		echo "clang-format not found. Install it or use the build script: scripts\\build-windows.bat --deps"; \
	fi
else
	@echo "Formatting source code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i; \
		echo "Code formatting completed successfully"; \
	else \
		echo "clang-format not found. Installing development tools..."; \
		$(MAKE) dev-deps; \
		echo "Please run 'make format' again after installation"; \
		exit 1; \
	fi
endif

# Check code style
check-style:
ifeq ($(PLATFORM),windows)
	@echo "Code style checking on Windows requires clang-format to be installed"
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror; \
	else \
		echo "clang-format not found. Install it or use the build script: scripts\\build-windows.bat --deps"; \
	fi
else
	@echo "Checking code style..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRC_DIR) $(INCLUDE_DIR) -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror; \
		echo "Code style check completed successfully"; \
	else \
		echo "clang-format not found. Installing development tools..."; \
		$(MAKE) dev-deps; \
		echo "Please run 'make check-style' again after installation"; \
		exit 1; \
	fi
endif

# Lint code
lint: check-style
ifeq ($(PLATFORM),windows)
	@echo "Code linting on Windows requires cppcheck to be installed"
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c++17 $(SRC_DIR) $(INCLUDE_DIR); \
	else \
		echo "cppcheck not found. Install it or use the build script: scripts\\build-windows.bat --deps"; \
	fi
else
	@echo "Running static analysis with cppcheck..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c++17 $(SRC_DIR) $(INCLUDE_DIR); \
		echo "Static analysis completed successfully"; \
	else \
		echo "cppcheck not found. Installing development tools..."; \
		$(MAKE) dev-deps; \
		echo "Please run 'make lint' again after installation"; \
		exit 1; \
	fi
endif

# Security scan
security-scan:
ifeq ($(PLATFORM),windows)
	@echo "Security scanning tools not yet implemented for Windows"
	@echo "Consider using the build script: scripts\\build-windows.bat --deps"
else
	@echo "Running security scan..."
	@if command -v bandit >/dev/null 2>&1 && command -v semgrep >/dev/null 2>&1; then \
		echo "Running bandit security scan..."; \
		bandit -r $(SRC_DIR); \
		echo "Running semgrep security scan..."; \
		semgrep --config=auto $(SRC_DIR); \
		echo "Security scan completed successfully"; \
	else \
		echo "Security scanning tools not found. Installing development tools..."; \
		$(MAKE) dev-deps; \
		echo "Please run 'make security-scan' again after installation"; \
		exit 1; \
	fi
endif

# Dependencies
deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing dependencies on macOS..."
	sudo port install openssl jsoncpp cmake
else ifeq ($(PLATFORM),linux)
	@echo "Installing dependencies on Linux..."
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev
	# For RPM-based systems
	# sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel
else ifeq ($(PLATFORM),windows)
	@echo "Installing dependencies on Windows..."
	@echo "Please run: scripts\\build-windows.bat --deps"
	@echo "This will install vcpkg and required dependencies"
endif

# Platform-specific dependency targets
macos-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing macOS-specific dependencies..."
	sudo port install openssl jsoncpp cmake pkgconfig
	@echo "macOS dependencies installed successfully"
else
	@echo "This target is only available on macOS"
endif

linux-deps:
ifeq ($(PLATFORM),linux)
	@echo "Installing Linux-specific dependencies..."
	@if command -v apt-get >/dev/null 2>&1; then \
		echo "Detected Debian/Ubuntu system..."; \
		sudo apt-get update; \
		sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev pkg-config; \
	elif command -v yum >/dev/null 2>&1; then \
		echo "Detected Red Hat/CentOS system..."; \
		sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel pkgconfig; \
	elif command -v dnf >/dev/null 2>&1; then \
		echo "Detected Fedora system..."; \
		sudo dnf install -y gcc-c++ cmake openssl-devel jsoncpp-devel pkgconfig; \
	elif command -v pacman >/dev/null 2>&1; then \
		echo "Detected Arch Linux system..."; \
		sudo pacman -S --needed base-devel cmake openssl jsoncpp pkgconf; \
	else \
		echo "Unsupported Linux distribution. Please install manually:"; \
		echo "  - build-essential/gcc-c++"; \
		echo "  - cmake"; \
		echo "  - libssl-dev/openssl-devel"; \
		echo "  - libjsoncpp-dev/jsoncpp-devel"; \
		echo "  - pkg-config/pkgconfig"; \
	fi
	@echo "Linux dependencies installed successfully"
else
	@echo "This target is only available on Linux"
endif

windows-deps:
ifeq ($(PLATFORM),windows)
	@echo "Installing Windows dependencies..."
	@if exist scripts\build-windows.bat ( \
		scripts\build-windows.bat --deps; \
	) else ( \
		echo "Windows build script not found. Please install manually:"; \
		echo "  - Visual Studio 2019 or later"; \
		echo "  - CMake"; \
		echo "  - vcpkg"; \
		echo "  - OpenSSL"; \
		echo "  - JsonCPP"; \
	)
else
	@echo "This target is only available on Windows"
endif

# Runtime dependencies (for running the service)
runtime-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing runtime dependencies on macOS..."
	sudo port install openssl
	@echo "Runtime dependencies installed successfully"
else ifeq ($(PLATFORM),linux)
	@echo "Installing runtime dependencies on Linux..."
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get install -y libssl3 libjsoncpp25; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y openssl jsoncpp; \
	elif command -v dnf >/dev/null 2>&1; then \
		sudo dnf install -y openssl jsoncpp; \
	elif command -v pacman >/dev/null 2>&1; then \
		sudo pacman -S --needed openssl jsoncpp; \
	else \
		echo "Please install runtime dependencies manually: openssl, jsoncpp"; \
	fi
	@echo "Runtime dependencies installed successfully"
else ifeq ($(PLATFORM),windows)
	@echo "Runtime dependencies on Windows are included with the application"
endif

# Development dependencies
dev-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing development tools on macOS..."
	@echo "Installing available packages from MacPorts..."
	sudo port install cppcheck py3-bandit semgrep
	@echo "Note: clang-format is not available in MacPorts."
	@echo "To install clang-format, you can:"
	@echo "  1. Use Homebrew: brew install clang-format"
	@echo "  2. Install Xcode Command Line Tools: xcode-select --install"
	@echo "  3. Install manually from LLVM releases"
else ifeq ($(PLATFORM),linux)
	@echo "Installing development tools on Linux..."
	sudo apt-get update
	sudo apt-get install -y clang-format cppcheck python3-pip
	pip3 install bandit semgrep
else ifeq ($(PLATFORM),windows)
	@echo "Installing development tools on Windows..."
	@echo "Please run: scripts\\build-windows.bat --dev-deps"
	@echo "This will install clang-format, cppcheck, and other development tools"
endif

# Alternative development dependencies (using Homebrew on macOS)
dev-deps-brew:
ifeq ($(PLATFORM),macos)
	@echo "Installing development tools on macOS using Homebrew..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install clang-format cppcheck; \
		pip3 install bandit semgrep; \
		echo "Development tools installed successfully via Homebrew"; \
	else \
		echo "Homebrew not found. Please install Homebrew first:"; \
		echo "  /bin/bash -c \"\$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""; \
	fi
else
	@echo "This target is only available on macOS"
endif

# Windows-specific targets
windows-service: install
ifeq ($(PLATFORM),windows)
	@echo "Creating Windows service..."
	scripts\build-windows.bat --service
else
	@echo "This target is only available on Windows"
endif

# Docker targets
docker-build:
	docker build -t $(PROJECT_NAME):$(VERSION) .

docker-run:
	docker run -d --name $(PROJECT_NAME)-$(VERSION) -p 123:123/udp $(PROJECT_NAME):$(VERSION)

docker-stop:
	docker stop $(PROJECT_NAME)-$(VERSION)
	docker rm $(PROJECT_NAME)-$(VERSION)

# Service management
service-install: install
ifeq ($(PLATFORM),macos)
	@echo "Installing service on macOS..."
	@if [ -f $(DEPLOYMENT_DIR)/launchd/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo cp $(DEPLOYMENT_DIR)/launchd/com.$(PROJECT_NAME).$(PROJECT_NAME).plist /Library/LaunchDaemons/; \
		sudo launchctl load /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist; \
		echo "Service installed and started successfully"; \
	else \
		echo "Service file not found at $(DEPLOYMENT_DIR)/launchd/com.$(PROJECT_NAME).$(PROJECT_NAME).plist"; \
		echo "Please create the service file first"; \
		exit 1; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Installing service on Linux..."
	@if [ -f $(DEPLOYMENT_DIR)/systemd/$(PROJECT_NAME).service ]; then \
		sudo cp $(DEPLOYMENT_DIR)/systemd/$(PROJECT_NAME).service /etc/systemd/system/; \
		sudo systemctl daemon-reload; \
		sudo systemctl enable $(PROJECT_NAME); \
		sudo systemctl start $(PROJECT_NAME); \
		echo "Service installed and started successfully"; \
	else \
		echo "Service file not found at $(DEPLOYMENT_DIR)/systemd/$(PROJECT_NAME).service"; \
		echo "Please create the service file first"; \
		exit 1; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Installing service on Windows..."
	@if exist scripts\build-windows.bat ( \
		scripts\build-windows.bat --service; \
	) else ( \
		echo "Windows build script not found. Please install service manually:"; \
		echo "  sc create SimpleNTPDaemon binPath= \"$(INSTALL_PREFIX)\\bin\\$(PROJECT_NAME).exe\""; \
		echo "  sc start SimpleNTPDaemon"; \
	)
endif

service-uninstall:
ifeq ($(PLATFORM),macos)
	@echo "Uninstalling service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl unload /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist; \
		sudo rm -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist; \
		echo "Service uninstalled successfully"; \
	else \
		echo "Service not found"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Uninstalling service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl stop $(PROJECT_NAME); \
		sudo systemctl disable $(PROJECT_NAME); \
		sudo rm -f /etc/systemd/system/$(PROJECT_NAME).service; \
		sudo systemctl daemon-reload; \
		echo "Service uninstalled successfully"; \
	else \
		echo "Service not found"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Uninstalling service on Windows..."
	@sc query SimpleNTPDaemon >nul 2>&1 && ( \
		sc stop SimpleNTPDaemon; \
		sc delete SimpleNTPDaemon; \
		echo "Service uninstalled successfully"; \
	) || echo "Service not found"
endif

service-status:
ifeq ($(PLATFORM),macos)
	@echo "Checking service status on macOS..."
	@if launchctl list | grep -q $(PROJECT_NAME); then \
		echo "Service is running:"; \
		launchctl list | grep $(PROJECT_NAME); \
	else \
		echo "Service is not running"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Checking service status on Linux..."
	@if systemctl is-active --quiet $(PROJECT_NAME); then \
		echo "Service is running:"; \
		sudo systemctl status $(PROJECT_NAME) --no-pager -l; \
	else \
		echo "Service is not running"; \
		@if systemctl is-enabled --quiet $(PROJECT_NAME); then \
			echo "Service is enabled but not running"; \
		else \
			echo "Service is not enabled"; \
		fi; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Checking service status on Windows..."
	@sc query SimpleNTPDaemon
endif

# Service control targets
service-start:
ifeq ($(PLATFORM),macos)
	@echo "Starting service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl start com.$(PROJECT_NAME).$(PROJECT_NAME); \
		echo "Service started successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Starting service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl start $(PROJECT_NAME); \
		echo "Service started successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Starting service on Windows..."
	@sc start SimpleNTPDaemon
endif

service-stop:
ifeq ($(PLATFORM),macos)
	@echo "Stopping service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl stop com.$(PROJECT_NAME).$(PROJECT_NAME); \
		echo "Service stopped successfully"; \
	else \
		echo "Service not installed"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Stopping service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl stop $(PROJECT_NAME); \
		echo "Service stopped successfully"; \
	else \
		echo "Service not installed"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Stopping service on Windows..."
	@sc stop SimpleNTPDaemon
endif

service-restart:
ifeq ($(PLATFORM),macos)
	@echo "Restarting service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl stop com.$(PROJECT_NAME).$(PROJECT_NAME); \
		sleep 2; \
		sudo launchctl start com.$(PROJECT_NAME).$(PROJECT_NAME); \
		echo "Service restarted successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Restarting service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl restart $(PROJECT_NAME); \
		echo "Service restarted successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Restarting service on Windows..."
	@sc stop SimpleNTPDaemon
	@timeout /t 2 /nobreak >nul
	@sc start SimpleNTPDaemon
endif

service-enable:
ifeq ($(PLATFORM),macos)
	@echo "Enabling service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl load -w /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist; \
		echo "Service enabled successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Enabling service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl enable $(PROJECT_NAME); \
		echo "Service enabled successfully"; \
	else \
		echo "Service not installed. Run 'make service-install' first"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Enabling service on Windows..."
	@sc config SimpleNTPDaemon start= auto
endif

service-disable:
ifeq ($(PLATFORM),macos)
	@echo "Disabling service on macOS..."
	@if [ -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist ]; then \
		sudo launchctl unload /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist; \
		echo "Service disabled successfully"; \
	else \
		echo "Service not installed"; \
	fi
else ifeq ($(PLATFORM),linux)
	@echo "Disabling service on Linux..."
	@if [ -f /etc/systemd/system/$(PROJECT_NAME).service ]; then \
		sudo systemctl disable $(PROJECT_NAME); \
		echo "Service disabled successfully"; \
	else \
		echo "Service not installed"; \
	fi
else ifeq ($(PLATFORM),windows)
	@echo "Disabling service on Windows..."
	@sc config SimpleNTPDaemon start= disabled
endif

# Legacy service targets for compatibility
start: service-start
stop: service-stop
restart: service-restart
status: service-status

# Configuration management
config-install: install
ifeq ($(PLATFORM),windows)
	$(MKDIR) "$(CONFIG_DIR)"
	$(CP) $(CONFIG_DIR_SRC)\* "$(CONFIG_DIR)\"
else
	sudo mkdir -p $(CONFIG_DIR)
	sudo cp -r $(CONFIG_DIR_SRC)/* $(CONFIG_DIR)/
	sudo chown -R root:root $(CONFIG_DIR)
	sudo chmod -R 644 $(CONFIG_DIR)
	sudo find $(CONFIG_DIR) -type d -exec chmod 755 {} \;
endif

config-backup:
	@$(MKDIR) $(DIST_DIR)/config-backup
	$(CP) $(CONFIG_DIR_SRC) $(DIST_DIR)/config-backup/
ifeq ($(PLATFORM),windows)
	powershell -Command "Compress-Archive -Path '$(DIST_DIR)\config-backup' -DestinationPath '$(DIST_DIR)\config-backup-$(VERSION).zip' -Force"
else
	tar -czf $(DIST_DIR)/config-backup-$(VERSION).tar.gz -C $(DIST_DIR) config-backup
endif

# Log management
log-rotate: install
ifeq ($(PLATFORM),linux)
	sudo cp $(DEPLOYMENT_DIR)/logrotate.d/$(PROJECT_NAME) /etc/logrotate.d/
	sudo chmod 644 /etc/logrotate.d/$(PROJECT_NAME)
else ifeq ($(PLATFORM),windows)
	@echo "Log rotation on Windows is handled by the Windows Event Log system"
	@echo "Configure in Event Viewer or use PowerShell commands"
else
	@echo "Log rotation not implemented for this platform"
endif

# Backup and restore
backup: config-backup
	@$(MKDIR) $(DIST_DIR)/backup
ifeq ($(PLATFORM),windows)
	powershell -Command "Compress-Archive -Path '$(CONFIG_DIR_SRC)', '$(DEPLOYMENT_DIR)', '$(SRC_DIR)', '$(INCLUDE_DIR)', 'CMakeLists.txt', 'Makefile', 'README.md', 'LICENSE' -DestinationPath '$(DIST_DIR)\$(PROJECT_NAME)-backup-$(VERSION).zip' -Force"
else
	tar -czf $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz \
		$(CONFIG_DIR_SRC) \
		$(DEPLOYMENT_DIR) \
		$(SRC_DIR) \
		$(INCLUDE_DIR) \
		CMakeLists.txt \
		Makefile \
		README.md \
		LICENSE
endif

restore: backup
	@echo "Restoring from backup..."
ifeq ($(PLATFORM),windows)
	@if exist $(DIST_DIR)\$(PROJECT_NAME)-backup-$(VERSION).zip ( \
		powershell -Command "Expand-Archive -Path '$(DIST_DIR)\$(PROJECT_NAME)-backup-$(VERSION).zip' -DestinationPath '.' -Force"; \
		echo Restore completed; \
	) else ( \
		echo No backup found at $(DIST_DIR)\$(PROJECT_NAME)-backup-$(VERSION).zip; \
	)
else
	@if [ -f $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz ]; then \
		tar -xzf $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz; \
		echo "Restore completed"; \
	else \
		echo "No backup found at $(DIST_DIR)/$(PROJECT_NAME)-backup-$(VERSION).tar.gz"; \
	fi
endif

# Cleanup
distclean: clean
	$(RM) $(DIST_DIR)
	$(RM) $(PACKAGE_DIR)
ifeq ($(PLATFORM),windows)
	for /r . %%i in (*.o *.a *.so *.dylib *.exe *.dll *.obj *.pdb *.ilk *.exp *.lib) do del "%%i" 2>nul
else
	find . -name "*.o" -delete
	find . -name "*.a" -delete
	find . -name "*.so" -delete
	find . -name "*.dylib" -delete
	find . -name "*.exe" -delete
	find . -name "*.dll" -delete
endif

# Legacy targets for compatibility
debug: dev-build
release: build
sanitize: dev-build
rebuild: clean build
test-verbose: test

# Help - Main help (most common targets)
help:
	@echo "Simple NTP Daemon - Main Help"
	@echo "============================="
	@echo ""
	@echo "Essential targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install the project"
	@echo "  uninstall        - Uninstall the project"
	@echo "  test             - Run tests"
	@echo "  package          - Build platform-specific packages"
	@echo "  package-source   - Create source code package"
	@echo ""
	@echo "Development targets:"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  format           - Format source code"
	@echo "  lint             - Run static analysis"
	@echo "  security-scan    - Run security scanning tools"
	@echo ""
	@echo "Dependency management:"
	@echo "  deps             - Install dependencies"
	@echo "  dev-deps         - Install development tools"
	@echo ""
	@echo "Service management:"
	@echo "  service-install  - Install system service"
	@echo "  service-status   - Check service status"
	@echo "  service-start    - Start service"
	@echo "  service-stop     - Stop service"
	@echo ""
	@echo "Help categories:"
	@echo "  help-all         - Show all available targets"
	@echo "  help-build       - Build and development targets"
	@echo "  help-package     - Package creation targets"
	@echo "  help-deps        - Dependency management targets"
	@echo "  help-service     - Service management targets"
	@echo "  help-docker      - Docker targets"
	@echo "  help-config      - Configuration management targets"
	@echo "  help-platform    - Platform-specific targets"
	@echo ""
	@echo "Examples:"
	@echo "  make build       - Build the project"
	@echo "  make test        - Build and run tests"
	@echo "  make package     - Create platform-specific packages"
	@echo "  make dev-deps    - Install development tools"
	@echo "  make help-all    - Show all available targets"

# Help - All targets (comprehensive)
help-all:
	@echo "Simple NTP Daemon - All Available Targets"
	@echo "========================================="
	@echo ""
	@echo "Essential targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install the project"
	@echo "  uninstall        - Uninstall the project"
	@echo "  test             - Run tests"
	@echo "  test-verbose     - Run tests with verbose output"
	@echo "  rebuild          - Clean and rebuild"
	@echo ""
	@echo "Package targets:"
	@echo "  package          - Build platform-specific packages"
	@echo "  package-source   - Create source code package"
	@echo "  package-script   - Build package using platform script"
	@echo "  package-all      - Build all package formats"
ifeq ($(PLATFORM),macos)
	@echo "  package-dmg      - Build DMG package (macOS only)"
	@echo "  package-pkg      - Build PKG package (macOS only)"
else ifeq ($(PLATFORM),linux)
	@echo "  package-rpm      - Build RPM package (Linux only)"
	@echo "  package-deb      - Build DEB package (Linux only)"
else ifeq ($(PLATFORM),windows)
	@echo "  package-msi      - Build MSI package (Windows only)"
	@echo "  package-zip      - Build ZIP package (Windows only)"
endif
	@echo ""
	@echo "Development targets:"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  debug            - Build with debug information"
	@echo "  release          - Build with optimization"
	@echo "  sanitize         - Build with sanitizers"
	@echo "  docs             - Build documentation"
	@echo "  analyze          - Run static analysis"
	@echo "  coverage         - Generate coverage report"
	@echo "  format           - Format source code"
	@echo "  check-style      - Check code style"
	@echo "  lint             - Run linting tools"
	@echo "  security-scan    - Run security scanning tools"
	@echo ""
	@echo "Dependency management:"
	@echo "  deps             - Install dependencies"
	@echo "  dev-deps         - Install development tools (MacPorts)"
	@echo "  dev-deps-brew    - Install development tools (Homebrew)"
	@echo "  macos-deps       - macOS-specific dependencies"
	@echo "  linux-deps       - Linux-specific dependencies"
	@echo "  windows-deps     - Windows-specific dependencies"
	@echo "  runtime-deps     - Runtime dependencies"
	@echo ""
	@echo "Service management:"
	@echo "  service-install  - Install system service"
	@echo "  service-uninstall- Uninstall system service"
	@echo "  service-status   - Check service status"
	@echo "  service-start    - Start service"
	@echo "  service-stop     - Stop service"
	@echo "  service-restart  - Restart service"
	@echo "  service-enable   - Enable service"
	@echo "  service-disable  - Disable service"
	@echo ""
	@echo "Docker targets:"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo ""
	@echo "Configuration management:"
	@echo "  config-install   - Install configuration files"
	@echo "  config-backup    - Backup configuration"
	@echo "  log-rotate       - Install log rotation"
	@echo "  backup           - Create full backup"
	@echo "  restore          - Restore from backup"
	@echo ""
	@echo "Cleanup targets:"
	@echo "  distclean        - Clean all generated files"
	@echo ""
	@echo "Legacy targets:"
	@echo "  start            - Start service (legacy)"
	@echo "  stop             - Stop service (legacy)"
	@echo "  restart          - Restart service (legacy)"
	@echo "  status           - Check service status (legacy)"

# Help - Build and development targets
help-build:
	@echo "Simple NTP Daemon - Build and Development Targets"
	@echo "================================================"
	@echo ""
	@echo "Essential build targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  rebuild          - Clean and rebuild"
	@echo ""
	@echo "Development build targets:"
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  debug            - Build with debug information"
	@echo "  release          - Build with optimization"
	@echo "  sanitize         - Build with sanitizers"
	@echo ""
	@echo "Testing targets:"
	@echo "  test             - Run tests"
	@echo "  test-verbose     - Run tests with verbose output"
	@echo ""
	@echo "Code quality targets:"
	@echo "  format           - Format source code"
	@echo "  check-style      - Check code style"
	@echo "  lint             - Run linting tools"
	@echo "  security-scan    - Run security scanning tools"
	@echo "  analyze          - Run static analysis"
	@echo "  coverage         - Generate coverage report"
	@echo ""
	@echo "Documentation:"
	@echo "  docs             - Build documentation"

# Help - Package creation targets
help-package:
	@echo "Simple NTP Daemon - Package Creation Targets"
	@echo "============================================"
	@echo ""
	@echo "Generic package targets:"
	@echo "  package          - Build platform-specific packages"
	@echo "  package-source   - Create source code package"
	@echo "  package-script   - Build package using platform script"
	@echo "  package-all      - Build all package formats"
	@echo ""
	@echo "Platform-specific package targets:"
ifeq ($(PLATFORM),macos)
	@echo "  package-dmg      - Build DMG package (macOS only)"
	@echo "  package-pkg      - Build PKG package (macOS only)"
	@echo ""
	@echo "Package Examples:"
	@echo "  make package      - Creates DMG and PKG files"
	@echo "  make package-dmg  - Creates only DMG file"
	@echo "  make package-pkg  - Creates only PKG file"
	@echo "  make package-all  - Creates DMG, PKG, and script-based packages"
else ifeq ($(PLATFORM),linux)
	@echo "  package-rpm      - Build RPM package (Linux only)"
	@echo "  package-deb      - Build DEB package (Linux only)"
	@echo ""
	@echo "Package Examples:"
	@echo "  make package      - Creates RPM and DEB files"
	@echo "  make package-rpm  - Creates only RPM file"
	@echo "  make package-deb  - Creates only DEB file"
	@echo "  make package-all  - Creates RPM, DEB, and script-based packages"
else ifeq ($(PLATFORM),windows)
	@echo "  package-msi      - Build MSI package (Windows only)"
	@echo "  package-zip      - Build ZIP package (Windows only)"
	@echo ""
	@echo "Package Examples:"
	@echo "  make package      - Creates MSI and ZIP files"
	@echo "  make package-msi  - Creates only MSI file"
	@echo "  make package-zip  - Creates only ZIP file"
	@echo "  make package-all  - Creates MSI, ZIP, and script-based packages"
endif
	@echo "  make package-source - Creates source code archive"

# Help - Dependency management targets
help-deps:
	@echo "Simple NTP Daemon - Dependency Management Targets"
	@echo "================================================"
	@echo ""
	@echo "General dependencies:"
	@echo "  deps             - Install dependencies"
	@echo "  runtime-deps     - Runtime dependencies"
	@echo ""
	@echo "Development dependencies:"
	@echo "  dev-deps         - Install development tools (MacPorts)"
	@echo "  dev-deps-brew    - Install development tools (Homebrew)"
	@echo ""
	@echo "Platform-specific dependencies:"
	@echo "  macos-deps       - macOS-specific dependencies"
	@echo "  linux-deps       - Linux-specific dependencies"
	@echo "  windows-deps     - Windows-specific dependencies"
	@echo ""
	@echo "Notes:"
	@echo "  - dev-deps uses MacPorts on macOS"
	@echo "  - dev-deps-brew uses Homebrew on macOS"
	@echo "  - linux-deps auto-detects distribution (apt, yum, dnf, pacman)"

# Help - Service management targets
help-service:
	@echo "Simple NTP Daemon - Service Management Targets"
	@echo "=============================================="
	@echo ""
	@echo "Service installation:"
	@echo "  service-install  - Install system service"
	@echo "  service-uninstall- Uninstall system service"
	@echo ""
	@echo "Service control:"
	@echo "  service-start    - Start service"
	@echo "  service-stop     - Stop service"
	@echo "  service-restart  - Restart service"
	@echo "  service-status   - Check service status"
	@echo ""
	@echo "Service configuration:"
	@echo "  service-enable   - Enable service (start on boot)"
	@echo "  service-disable  - Disable service (don't start on boot)"
	@echo ""
	@echo "Legacy targets (for compatibility):"
	@echo "  start            - Start service (legacy)"
	@echo "  stop             - Stop service (legacy)"
	@echo "  restart          - Restart service (legacy)"
	@echo "  status           - Check service status (legacy)"
	@echo ""
	@echo "Platform support:"
	@echo "  - macOS: launchd service"
	@echo "  - Linux: systemd service"
	@echo "  - Windows: Windows service"

# Help - Docker targets
help-docker:
	@echo "Simple NTP Daemon - Docker Targets"
	@echo "=================================="
	@echo ""
	@echo "Docker image management:"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo ""
	@echo "Usage examples:"
	@echo "  make docker-build - Build the Docker image"
	@echo "  make docker-run   - Start the NTP daemon in a container"
	@echo "  make docker-stop  - Stop and remove the container"

# Help - Configuration management targets
help-config:
	@echo "Simple NTP Daemon - Configuration Management Targets"
	@echo "==================================================="
	@echo ""
	@echo "Configuration installation:"
	@echo "  config-install   - Install configuration files"
	@echo "  config-backup    - Backup configuration"
	@echo ""
	@echo "Log management:"
	@echo "  log-rotate       - Install log rotation"
	@echo ""
	@echo "Backup and restore:"
	@echo "  backup           - Create full backup"
	@echo "  restore          - Restore from backup"
	@echo ""
	@echo "Usage examples:"
	@echo "  make config-install - Install configuration files to system location"
	@echo "  make config-backup  - Create backup of current configuration"
	@echo "  make backup         - Create full project backup"

# Help - Platform-specific targets
help-platform:
	@echo "Simple NTP Daemon - Platform-Specific Targets"
	@echo "============================================="
	@echo ""
ifeq ($(PLATFORM),macos)
	@echo "macOS-specific targets:"
	@echo "  macos-deps       - macOS dependencies via MacPorts"
	@echo "  dev-deps         - Development tools via MacPorts"
	@echo "  dev-deps-brew    - Development tools via Homebrew"
	@echo "  package-dmg      - Build DMG package"
	@echo "  package-pkg      - Build PKG package"
	@echo "  package-script   - Build using build-macos.sh"
	@echo ""
	@echo "macOS service management:"
	@echo "  - Uses launchd for service management"
	@echo "  - Service files installed to /Library/LaunchDaemons/"
else ifeq ($(PLATFORM),linux)
	@echo "Linux-specific targets:"
	@echo "  linux-deps       - Linux dependencies (auto-detects distribution)"
	@echo "  package-rpm      - Build RPM package"
	@echo "  package-deb      - Build DEB package"
	@echo "  package-script   - Build using build-linux.sh"
	@echo ""
	@echo "Supported Linux distributions:"
	@echo "  - Debian/Ubuntu (apt-get)"
	@echo "  - Red Hat/CentOS (yum)"
	@echo "  - Fedora (dnf)"
	@echo "  - Arch Linux (pacman)"
	@echo ""
	@echo "Linux service management:"
	@echo "  - Uses systemd for service management"
	@echo "  - Service files installed to /etc/systemd/system/
else ifeq ($(PLATFORM),windows)
	@echo "Windows-specific targets:"
	@echo "  windows-deps     - Windows dependencies"
	@echo "  windows-service  - Create Windows service"
	@echo "  package-msi      - Build MSI package"
	@echo "  package-zip      - Build ZIP package"
	@echo "  package-script   - Build using build-windows.bat"
	@echo ""
	@echo "Windows service management:"
	@echo "  - Uses Windows Service Control Manager"
	@echo "  - Service name: SimpleNTPDaemon"
endif

# Phony targets
.PHONY: all build clean install uninstall test package package-source package-rpm package-deb package-dmg package-pkg package-msi package-zip package-script package-all \
        dev-build dev-test docs analyze coverage format check-style lint security-scan deps dev-deps dev-deps-brew windows-deps windows-service \
        docker-build docker-run docker-stop service-install service-uninstall service-status service-start service-stop service-restart service-enable service-disable \
        config-install config-backup log-rotate backup restore distclean help help-all help-build help-package help-deps help-service help-docker help-config help-platform \
        debug release sanitize rebuild test-verbose start stop restart status

# Default target
.DEFAULT_GOAL := all
