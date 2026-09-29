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
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "kugelmatch";
        version = "1.2.6";
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
        ];

        installPhase = ''
          runHook preInstall
          mkdir -p $out/bin $out/share/kugelmatch
          cp kugelmatch $out/bin/
          cp -r $src/shaders $out/share/kugelmatch/
          runHook postInstall
        '';
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
        ];
      };
    };
}
