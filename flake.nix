# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
{
  description = "KugelMatch — raytraced Pong (CPU + GPU fragment-shader backends)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      kugelmatch = pkgs.stdenv.mkDerivation {
        pname = "kugelmatch";
        version = "1.2.13";
        src = ./.;

        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config
        ];

        buildInputs = with pkgs; [
          SDL2
          libGL
          libglvnd
        ];

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Release"
          "-DKUGELMATCH_NATIVE=OFF"
        ];

        # CMake install rules place bin, man, desktop, icon, metainfo, shaders
      };

      # Helper scripts for the dev shell (build into /tmp/kugelmatch-build).
      kugelmatch-configure = pkgs.writeShellScriptBin "kugelmatch-configure" ''
        set -euo pipefail
        ROOT="''${KUGELMATCH_SRC:-}"
        if [ -z "$ROOT" ]; then
          if ROOT_GIT="$(git rev-parse --show-toplevel 2>/dev/null)"; then
            ROOT="$ROOT_GIT"
          else
            ROOT="$(pwd)"
          fi
        fi
        BUILD="''${KUGELMATCH_BUILD:-/tmp/kugelmatch-build}"
        TYPE="''${KUGELMATCH_BUILD_TYPE:-Release}"
        echo "Source:  $ROOT"
        echo "Build:   $BUILD"
        echo "Type:    $TYPE"
        exec ${pkgs.cmake}/bin/cmake -S "$ROOT" -B "$BUILD" \
          -DCMAKE_BUILD_TYPE="$TYPE" \
          -DKUGELMATCH_NATIVE=ON \
          "$@"
      '';

      kugelmatch-run = pkgs.writeShellScriptBin "kugelmatch-run" ''
        set -euo pipefail
        ROOT="''${KUGELMATCH_SRC:-}"
        if [ -z "$ROOT" ]; then
          if ROOT_GIT="$(git rev-parse --show-toplevel 2>/dev/null)"; then
            ROOT="$ROOT_GIT"
          else
            ROOT="$(pwd)"
          fi
        fi
        BUILD="''${KUGELMATCH_BUILD:-/tmp/kugelmatch-build}"
        TYPE="''${KUGELMATCH_BUILD_TYPE:-Release}"
        JOBS="$(nproc 2>/dev/null || echo 4)"
        if [ ! -f "$BUILD/build.ninja" ] && [ ! -f "$BUILD/Makefile" ]; then
          echo "Configuring $BUILD..."
          ${pkgs.cmake}/bin/cmake -S "$ROOT" -B "$BUILD" \
            -DCMAKE_BUILD_TYPE="$TYPE" \
            -DKUGELMATCH_NATIVE=ON
        fi
        echo "Building in $BUILD..."
        ${pkgs.cmake}/bin/cmake --build "$BUILD" -j"$JOBS"
        BIN="$BUILD/kugelmatch"
        if [ ! -x "$BIN" ]; then
          echo "error: expected binary at $BIN" >&2
          exit 1
        fi
        cd "$ROOT"
        echo "Running: $BIN $*"
        exec "$BIN" "$@"
      '';
    in
    {
      packages.${system} = {
        default = kugelmatch;
        kugelmatch = kugelmatch;
      };

      apps.${system} = {
        default = {
          type = "app";
          program = "${kugelmatch}/bin/kugelmatch";
        };
        kugelmatch = {
          type = "app";
          program = "${kugelmatch}/bin/kugelmatch";
        };
        # GPU backend convenience launcher
        kugelmatch-gpu = {
          type = "app";
          program = "${pkgs.writeShellScript "kugelmatch-gpu" ''
            exec ${kugelmatch}/bin/kugelmatch --gpu "$@"
          ''}";
        };
      };

      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          pkg-config
          SDL2
          libGL
          libglvnd
          gcc
          gdb
          clang-tools
          kugelmatch-configure
          kugelmatch-run
        ];

        # When entering from the flake, point scripts at this source tree.
        KUGELMATCH_SRC = toString self;
        KUGELMATCH_BUILD = "/tmp/kugelmatch-build";

        shellHook = ''
          echo "KugelMatch dev shell"
          echo "  kugelmatch-configure   # cmake -S \$KUGELMATCH_SRC -B /tmp/kugelmatch-build"
          echo "  kugelmatch-run [--gpu] # build + run from source tree"
          echo "  KUGELMATCH_SRC=$KUGELMATCH_SRC"
          echo "  KUGELMATCH_BUILD=$KUGELMATCH_BUILD"
        '';
      };
    };
}
