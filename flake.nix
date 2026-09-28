{
  description = "Raytraced Pong - SDL2 software raytracer with checkerboard + mirror ball";

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
        version = "1.0.0";
        src = ./.;

        nativeBuildInputs = with pkgs; [
          cmake
          pkg-config
        ];

        buildInputs = with pkgs; [
          SDL2
        ];

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Release"
        ];

        installPhase = ''
          mkdir -p $out/bin
          cp kugelmatch $out/bin/
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          pkg-config
          SDL2
          gcc
          gdb
          clang-tools
        ];
      };
    };
}
