# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
#
# Convenience wrapper around CMake (non-Nix builds).
# Default local build dir is ./build; scripts use /tmp/kugelmatch-build.
BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= $(shell nproc 2>/dev/null || echo 4)
PREFIX ?= /usr/local
DESTDIR ?=

.PHONY: all configure build install uninstall run-cpu run-gpu clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX) -DKUGELMATCH_NATIVE=ON

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

install: build
	DESTDIR=$(DESTDIR) cmake --install $(BUILD_DIR)

uninstall:
	@echo "Remove files listed by: cmake --install $(BUILD_DIR) --prefix $(PREFIX)  (manual)"
	rm -f $(DESTDIR)$(PREFIX)/bin/kugelmatch
	rm -f $(DESTDIR)$(PREFIX)/share/applications/kugelmatch.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/kugelmatch.svg
	rm -f $(DESTDIR)$(PREFIX)/share/man/man6/kugelmatch.6
	rm -f $(DESTDIR)$(PREFIX)/share/metainfo/org.kugelmatch.KugelMatch.metainfo.xml
	rm -rf $(DESTDIR)$(PREFIX)/share/kugelmatch

run-cpu: build
	./$(BUILD_DIR)/kugelmatch --cpu

run-gpu: build
	./$(BUILD_DIR)/kugelmatch --gpu

clean:
	rm -rf $(BUILD_DIR)
