# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
#
# Convenience wrapper around CMake (non-Nix builds).
BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all configure build run-cpu run-gpu clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

run-cpu: build
	./$(BUILD_DIR)/kugelmatch --cpu

run-gpu: build
	./$(BUILD_DIR)/kugelmatch --gpu

clean:
	rm -rf $(BUILD_DIR)
