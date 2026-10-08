# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# Windows builds, cross-compiled with MinGW (pkgsCross). SDL2 is the official
# prebuilt MinGW package (SDL2-win32 flake).
#
#   nix build .#kugelmatch-win64
#   nix build .#kugelmatch-win64-zip

{ pkgs
, sdl2Win64
, sdl2Win32
, version
}:

let
  lib = pkgs.lib;

  mkGame = { crossPkgs, sdl2, arch }:
    crossPkgs.stdenv.mkDerivation {
      pname = "kugelmatch-${arch}-build";
      inherit version;
      src = lib.cleanSource ../.;
      nativeBuildInputs = [ pkgs.cmake ];
      buildInputs = [ sdl2 crossPkgs.windows.mcfgthreads ];
      cmakeFlags = [
        "-DCMAKE_BUILD_TYPE=Release"
        "-DPROJECT_VERSION_FULL=${version}"
        "-DKUGELMATCH_NATIVE=OFF"
        "-DKUGELMATCH_OPENGLES2=ON"
        "-DSDL2_DIR=${sdl2}/lib/cmake/SDL2"
      ];
    };

  mkFolder = { crossPkgs, sdl2, arch }:
    let game = mkGame { inherit crossPkgs sdl2 arch; };
    in pkgs.runCommand "kugelmatch-${arch}-${version}" { } ''
      mkdir -p $out
      cp ${game}/bin/kugelmatch.exe $out/
      cp ${sdl2}/bin/SDL2.dll $out/
      cp ${crossPkgs.windows.mcfgthreads}/bin/libmcfgthread-[0-9]*.dll $out/
      cp ${../LICENSES/GPL-3.0-or-later.txt} $out/COPYING.txt
      cat > $out/README.txt <<'EOR'
      KugelMatch ${version}
      Raytraced Pong (OpenGL ES 2.0 GPU raytracer by default; --cpu for software).

      Keys: A/D or arrows move, Space start, P pause, F8 CPU/GPU, F11 fullscreen.
      Settings: %APPDATA%\grumbel\kugelmatch\config.cfg

      Licence: GPL-3.0-or-later (COPYING.txt).
      Source: https://github.com/Grumbel/kugelmatch
      EOR
      sed -i 's/^      //; s/$/\r/' $out/README.txt
    '';

  mkZip = { folder, arch }:
    pkgs.runCommand "kugelmatch-${version}-${arch}.zip" { nativeBuildInputs = [ pkgs.zip ]; } ''
      mkdir -p $out
      work=$(mktemp -d)
      cp -r --no-preserve=mode ${folder} "$work/kugelmatch-${version}-${arch}"
      cd "$work"
      zip -r "$out/kugelmatch-${version}-${arch}.zip" "kugelmatch-${version}-${arch}"
    '';

  win64Folder = mkFolder {
    crossPkgs = pkgs.pkgsCross.mingwW64;
    sdl2 = sdl2Win64;
    arch = "win64";
  };
  win32Folder = mkFolder {
    crossPkgs = pkgs.pkgsCross.mingw32;
    sdl2 = sdl2Win32;
    arch = "win32";
  };
in {
  kugelmatch-win64 = win64Folder;
  kugelmatch-win32 = win32Folder;
  kugelmatch-win64-zip = mkZip { folder = win64Folder; arch = "win64"; };
  kugelmatch-win32-zip = mkZip { folder = win32Folder; arch = "win32"; };
}
