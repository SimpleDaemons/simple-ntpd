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
	sudo port install openssl jsoncpp cmake
else ifeq ($(PLATFORM),linux)
	sudo apt-get update
	sudo apt-get install -y build-essential cmake libssl-dev libjsoncpp-dev
	# For RPM-based systems
	# sudo yum install -y gcc-c++ cmake openssl-devel jsoncpp-devel
else ifeq ($(PLATFORM),windows)
	@echo "Installing dependencies on Windows..."
	@echo "Please run: scripts\\build-windows.bat --deps"
	@echo "This will install vcpkg and required dependencies"
endif

# Development dependencies
dev-deps:
ifeq ($(PLATFORM),macos)
	@echo "Installing development tools on macOS..."
	sudo port install clang-format cppcheck py3-bandit semgrep
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

# Windows-specific targets
windows-deps:
ifeq ($(PLATFORM),windows)
	@echo "Installing Windows dependencies..."
	scripts\build-windows.bat --deps
else
	@echo "This target is only available on Windows"
endif

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
	sudo cp $(DEPLOYMENT_DIR)/launchd/com.$(PROJECT_NAME).$(PROJECT_NAME).plist /Library/LaunchDaemons/
	sudo launchctl load /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
else ifeq ($(PLATFORM),linux)
	sudo cp $(DEPLOYMENT_DIR)/systemd/$(PROJECT_NAME).service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable $(PROJECT_NAME)
	sudo systemctl start $(PROJECT_NAME)
else ifeq ($(PLATFORM),windows)
	@echo "Windows service creation..."
	scripts\build-windows.bat --service
endif

service-uninstall:
ifeq ($(PLATFORM),macos)
	sudo launchctl unload /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
	sudo rm -f /Library/LaunchDaemons/com.$(PROJECT_NAME).$(PROJECT_NAME).plist
else ifeq ($(PLATFORM),linux)
	sudo systemctl stop $(PROJECT_NAME)
	sudo systemctl disable $(PROJECT_NAME)
	sudo rm -f /etc/systemd/system/$(PROJECT_NAME).service
	sudo systemctl daemon-reload
else ifeq ($(PLATFORM),windows)
	@echo "Removing Windows service..."
	sc delete SimpleNTPDaemon
endif

service-status:
ifeq ($(PLATFORM),macos)
	launchctl list | grep $(PROJECT_NAME)
else ifeq ($(PLATFORM),linux)
	sudo systemctl status $(PROJECT_NAME)
else ifeq ($(PLATFORM),windows)
	sc query SimpleNTPDaemon
endif

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

# Help
help:
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  build            - Build using CMake"
	@echo "  clean            - Clean build files"
	@echo "  install          - Install the project"
	@echo "  uninstall        - Uninstall the project"
	@echo "  test             - Run tests"
	@echo "  package          - Build platform-specific packages (DMG+PKG/RPM+DEB/MSI+ZIP)"
	@echo "  package-source   - Create source code package"
	@echo "  package-script   - Build package using platform script"
	@echo "  package-all      - Build all package formats"
	@echo ""
	@echo "Platform-specific package targets:"
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
	@echo "  dev-build        - Build in debug mode"
	@echo "  dev-test         - Run tests in debug mode"
	@echo "  docs             - Build documentation"
	@echo "  analyze          - Run static analysis"
	@echo "  coverage         - Generate coverage report"
	@echo "  format           - Format source code"
	@echo "  check-style      - Check code style"
	@echo "  lint             - Run linting tools"
	@echo "  security-scan    - Run security scanning tools"
	@echo "  deps             - Install dependencies"
	@echo "  dev-deps         - Install development tools (clang-format, cppcheck, etc.)"
	@echo "  windows-deps     - Install Windows dependencies"
	@echo "  windows-service  - Create Windows service"
	@echo "  docker-build     - Build Docker image"
	@echo "  docker-run       - Run Docker container"
	@echo "  docker-stop      - Stop Docker container"
	@echo "  service-install  - Install system service"
	@echo "  service-uninstall- Uninstall system service"
	@echo "  service-status   - Check service status"
	@echo "  config-install   - Install configuration files"
	@echo "  config-backup    - Backup configuration"
	@echo "  log-rotate       - Install log rotation"
	@echo "  backup           - Create full backup"
	@echo "  restore          - Restore from backup"
	@echo "  distclean        - Clean all generated files"
	@echo "  help             - Show this help message"
	@echo ""
	@echo ""
	@echo "Platform-specific targets:"
ifeq ($(PLATFORM),windows)
	@echo "  Windows-specific: windows-deps, windows-service, package-msi, package-zip"
else ifeq ($(PLATFORM),macos)
	@echo "  macOS-specific: package-script (uses build-macos.sh)"
else ifeq ($(PLATFORM),linux)
	@echo "  Linux-specific: package-rpm, package-deb, package-script (uses build-linux.sh)"
endif
	@echo ""
	@echo "Examples:"
	@echo "  make build        - Build the project"
	@echo "  make test         - Build and run tests"
	@echo "  make install      - Build and install"
	@echo "  make package      - Create platform-specific packages"
	@echo "  make dev-deps     - Install development tools"
	@echo "  make format       - Format source code"
	@echo "  make lint         - Run static analysis"
	@echo "  make security-scan- Run security scanning"
	@echo ""
	@echo "Package Examples:"
ifeq ($(PLATFORM),macos)
	@echo "  make package      - Creates DMG and PKG files"
	@echo "  make package-dmg  - Creates only DMG file"
	@echo "  make package-pkg  - Creates only PKG file"
	@echo "  make package-all  - Creates DMG, PKG, and script-based packages"
else ifeq ($(PLATFORM),linux)
	@echo "  make package      - Creates RPM and DEB files"
	@echo "  make package-rpm  - Creates only RPM file"
	@echo "  make package-deb  - Creates only DEB file"
	@echo "  make package-all  - Creates RPM, DEB, and script-based packages"
else ifeq ($(PLATFORM),windows)
	@echo "  make package      - Creates MSI and ZIP files"
	@echo "  make package-msi  - Creates only MSI file"
	@echo "  make package-zip  - Creates only ZIP file"
	@echo "  make package-all  - Creates MSI, ZIP, and script-based packages"
endif
	@echo "  make package-source - Creates source code archive"

# Phony targets
.PHONY: all build clean install uninstall test package package-source package-rpm package-deb package-dmg package-pkg package-msi package-zip package-script package-all \
        dev-build dev-test docs analyze coverage format check-style lint security-scan deps dev-deps windows-deps windows-service \
        docker-build docker-run docker-stop service-install service-uninstall service-status \
        config-install config-backup log-rotate backup restore distclean help debug release sanitize rebuild test-verbose

# Default target
.DEFAULT_GOAL := all
