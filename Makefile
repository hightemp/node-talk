# NodeTalk Makefile — convenience wrappers around CMake + git.
#
# Usage:
#   make              # configure + build (Release)
#   make debug        # configure + build (Debug)
#   make run          # build then launch
#   make test         # build then ctest
#   make clean        # wipe build dir
#   make distclean    # wipe build + packaging artifacts
#   make package      # build + cpack (TGZ; on Linux also DEB)
#   make appimage     # Linux only: build the .AppImage
#   make install      # cmake --install into $(PREFIX)
#   make format       # clang-format all C++ sources
#   make lupdate      # refresh translation .ts files
#   make version      # print current VERSION
#   make tag          # create annotated tag v$(VERSION) (force)
#   make release      # bump build, tag v$(VERSION) -f, push -f
#
# Variables you can override on the command line:
#   BUILD_DIR=build BUILD_TYPE=Release JOBS=8 PREFIX=/usr/local
#   CMAKE_ARGS="-DNODETALK_BUILD_TESTS=OFF"
#   REMOTE=origin BRANCH=main

SHELL        := /bin/bash
.SHELLFLAGS  := -eu -o pipefail -c
.DEFAULT_GOAL := all

# ----- Config ---------------------------------------------------------------
VERSION      := $(strip $(shell cat VERSION.txt 2>/dev/null))
ifeq ($(VERSION),)
$(error VERSION.txt file is empty or missing)
endif
TAG          := v$(VERSION)

BUILD_DIR    ?= build
BUILD_TYPE   ?= Release
JOBS         ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
PREFIX       ?= /usr/local
CMAKE        ?= cmake
CTEST        ?= ctest
GIT          ?= git
REMOTE       ?= origin
BRANCH       ?= $(shell $(GIT) rev-parse --abbrev-ref HEAD 2>/dev/null || echo main)

CMAKE_ARGS   ?=
CONFIGURE_ARGS = -S . -B $(BUILD_DIR) \
                 -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
                 -DNODETALK_VERSION_OVERRIDE=$(VERSION) \
                 $(CMAKE_ARGS)

UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)

# ----- Phony ----------------------------------------------------------------
.PHONY: all configure build debug run test clean distclean \
        package appimage install format lupdate \
        version tag release help check-clean

# ----- Targets --------------------------------------------------------------
all: build

help:
	@grep -E '^[a-zA-Z_-]+:.*?##' $(MAKEFILE_LIST) | \
		awk 'BEGIN{FS=":.*?##"};{printf "  \033[36m%-14s\033[0m %s\n",$$1,$$2}'

configure: ## Configure CMake build directory
	$(CMAKE) $(CONFIGURE_ARGS)

build: configure ## Build (Release by default)
	$(CMAKE) --build $(BUILD_DIR) --config $(BUILD_TYPE) --parallel $(JOBS)

debug: ## Build in Debug mode
	$(MAKE) build BUILD_TYPE=Debug

run: build ## Build then launch the app
	@if [ -x $(BUILD_DIR)/NodeTalk ]; then \
		$(BUILD_DIR)/NodeTalk; \
	elif [ -d $(BUILD_DIR)/NodeTalk.app ]; then \
		open $(BUILD_DIR)/NodeTalk.app; \
	else \
		$(BUILD_DIR)/NodeTalk.exe; \
	fi

test: build ## Run ctest
	cd $(BUILD_DIR) && QT_QPA_PLATFORM=offscreen $(CTEST) --output-on-failure -j $(JOBS)

install: build ## Install into $(PREFIX)
	$(CMAKE) --install $(BUILD_DIR) --prefix $(PREFIX) --config $(BUILD_TYPE)

clean: ## Remove the build directory
	rm -rf $(BUILD_DIR)

distclean: clean ## Remove build + packaging artifacts
	rm -rf NodeTalk-*.AppImage NodeTalk-*.zip NodeTalk-*.dmg \
	       NodeTalk-*.tar.gz NodeTalk-*.deb Output/

package: build ## Build + run cpack
ifeq ($(UNAME_S),Linux)
	cd $(BUILD_DIR) && cpack -G "TGZ;DEB"
else ifeq ($(UNAME_S),Darwin)
	cd $(BUILD_DIR) && cpack -G "DragNDrop;TGZ"
else
	cd $(BUILD_DIR) && cpack -G "ZIP"
endif

appimage: build ## Build a Linux .AppImage (linuxdeploy required)
	VERSION=$(VERSION) BUILD_DIR=$(BUILD_DIR) bash packaging/linux/AppImage.sh

format: ## Run clang-format on all C++ sources
	@find src tests -type f \( -name '*.cpp' -o -name '*.h' \) -print0 | \
		xargs -0 clang-format -i

lupdate: ## Refresh translation .ts files from sources
	$(CMAKE) --build $(BUILD_DIR) --target nodetalk_core_lupdate || \
		$(CMAKE) --build $(BUILD_DIR) --target update_translations

version: ## Print current version
	@echo $(VERSION)

check-clean:
	@if ! $(GIT) diff --quiet || ! $(GIT) diff --cached --quiet; then \
		echo "warning: working tree has uncommitted changes (continuing because release uses -f)"; \
	fi

tag: ## Create / move annotated tag v$(VERSION) (force)
	$(GIT) tag -fa $(TAG) -m "NodeTalk $(VERSION)"

release: check-clean ## Tag v$(VERSION) -f and push -f to $(REMOTE)/$(BRANCH)
	@echo ">> Releasing NodeTalk $(VERSION) -> $(TAG)"
	$(GIT) add VERSION.txt
	@if ! $(GIT) diff --cached --quiet; then \
		$(GIT) commit -m "release: $(VERSION)"; \
	else \
		echo "VERSION already committed"; \
	fi
	$(GIT) tag -fa $(TAG) -m "NodeTalk $(VERSION)"
	$(GIT) push -f $(REMOTE) $(BRANCH)
	$(GIT) push -f $(REMOTE) $(TAG)
	@echo ">> Pushed $(TAG); GitHub Release workflow will build artifacts."
