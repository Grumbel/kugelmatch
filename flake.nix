# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

{
  description = "KugelMatch — raytraced Pong (CPU + GLES2 GPU backends)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2-win32.inputs.flake-utils.follows = "flake-utils";

    sdl2-src = {
      url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, SDL2-win32, sdl2-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        lib = nixpkgs.lib;

        versionBase = lib.strings.fileContents ./VERSION;
        gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
        isDev = lib.strings.hasInfix "-dev" versionBase;
        version =
          if isDev then
            "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
          else
            versionBase;

        kugelmatch = pkgs.stdenv.mkDerivation {
          pname = "kugelmatch";
          inherit version;
          src = lib.cleanSource ./.;
          nativeBuildInputs = with pkgs; [ cmake pkg-config ];
          buildInputs = with pkgs; [ SDL2 libGL libglvnd ];
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            "-DKUGELMATCH_NATIVE=OFF"
            "-DKUGELMATCH_OPENGLES2=OFF"
            "-DPROJECT_VERSION_FULL=${version}"
          ];
        };

        windows = import ./nix/windows.nix {
          inherit pkgs version;
          sdl2Win64 = SDL2-win32.packages.${system}.SDL2-win64;
          sdl2Win32 = SDL2-win32.packages.${system}.SDL2-win32;
        };

        lastModified = self.lastModifiedDate or "19700101000000";
        date = "${builtins.substring 0 4 lastModified}-${builtins.substring 4 2 lastModified}-${builtins.substring 6 2 lastModified}";

        wasm = import ./nix/wasm.nix {
          inherit pkgs version gitRev date;
          sdlSrc = sdl2-src;
          sdlVersion = "2.30.3";
        };

        r36s = import ./nix/r36s.nix { inherit pkgs version date; };

        androidPkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          config.android_sdk.accept_license = true;
        };
        android = import ./nix/android.nix {
          pkgs = androidPkgs;
          sdlSrc = sdl2-src;
          sdlVersion = "2.30.3";
          buildToolsVersion = "30.0.3";
          packagePlatform = "22";
          compilePlatform = "33";
          targetAbis = [ "armeabi-v7a" "arm64-v8a" "x86_64" ];
          androidSdk = (androidPkgs.androidenv.composeAndroidPackages {
            platformVersions = [ "22" "33" ];
            buildToolsVersions = [ "30.0.3" ];
            includeNDK = true;
            ndkVersion = "29.0.14206865";
            includeEmulator = false;
            includeSources = false;
          }).androidsdk;
          inherit version;
          versionCode = self.revCount or 1;
        };


        kugelmatch-gles2 = pkgs.stdenv.mkDerivation {
          pname = "kugelmatch-gles2";
          inherit version;
          src = lib.cleanSource ./.;
          nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config ];
          buildInputs = [ pkgs.SDL2 pkgs.libGL pkgs.libglvnd ];
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            "-DKUGELMATCH_NATIVE=ON"
            "-DKUGELMATCH_OPENGLES2=ON"
          ];
          postInstall = ''
            if [ -f "$out/bin/kugelmatch" ]; then
              mv "$out/bin/kugelmatch" "$out/bin/kugelmatch-gles2"
            fi
          '';
        };

        kugelmatch-configure = pkgs.writeShellScriptBin "kugelmatch-configure" ''
          set -euo pipefail
          ROOT="''${KUGELMATCH_SRC:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
          BUILD="''${KUGELMATCH_BUILD:-/tmp/kugelmatch-build}"
          TYPE="''${KUGELMATCH_BUILD_TYPE:-Release}"
          echo "Source: $ROOT  Build: $BUILD  Type: $TYPE"
          exec ${pkgs.cmake}/bin/cmake -S "$ROOT" -B "$BUILD" \
            -DCMAKE_BUILD_TYPE="$TYPE" \
            -DKUGELMATCH_NATIVE=ON \
            -DKUGELMATCH_OPENGLES2=OFF \
            "$@"
        '';

        kugelmatch-run = pkgs.writeShellScriptBin "kugelmatch-run" ''
          set -euo pipefail
          ROOT="''${KUGELMATCH_SRC:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
          BUILD="''${KUGELMATCH_BUILD:-/tmp/kugelmatch-build}"
          if [ ! -f "$BUILD/Makefile" ] && [ ! -f "$BUILD/build.ninja" ]; then
            kugelmatch-configure || exit 1
          fi
          cmake --build "$BUILD" -j"$(nproc)" || exit 1
          exec "$BUILD/kugelmatch" "$@"
        '';
      in {
        packages = {
          default = kugelmatch;
          kugelmatch = kugelmatch;
          kugelmatch-gpu = kugelmatch;
          kugelmatch-gles2 = kugelmatch-gles2;
          kugelmatch-win64 = windows.kugelmatch-win64;
          kugelmatch-win32 = windows.kugelmatch-win32;
          kugelmatch-win64-zip = windows.kugelmatch-win64-zip;
          kugelmatch-win32-zip = windows.kugelmatch-win32-zip;
          kugelmatch-wasm = wasm.kugelmatchWasm;
          kugelmatch-r36s = r36s.portmaster;
          kugelmatch-r36s-zip = r36s.portmasterZip;
          kugelmatch-android = android.apk;
        };

        # nix run .#kugelmatch-wasm  → serve the site and open a browser (like kurvenrausch)
        apps = {
          default = {
            type = "app";
            program = "${kugelmatch}/bin/kugelmatch";
          };
          kugelmatch-wasm = wasm.serveApp;
          install-android-kugelmatch = android.installApp;
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake pkg-config SDL2 SDL2.dev libGL libglvnd
            clang-tools ccache
            kugelmatch-configure kugelmatch-run
          ];
          shellHook = ''
            export KUGELMATCH_SRC="$(pwd)"
            export KUGELMATCH_BUILD="''${KUGELMATCH_BUILD:-/tmp/kugelmatch-build}"
            echo "KugelMatch dev shell — kugelmatch-configure / kugelmatch-run"
          '';
        };
      });
}
