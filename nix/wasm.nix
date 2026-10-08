# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

# The WebAssembly build: the game compiled with Emscripten into a web page
# (index.html / kugelmatch.html, kugelmatch.js, kugelmatch.wasm),
# following Pingus and SuperTux Origins. Everything is built offline: SDL2
# from its release tarball (a flake input) instead of Emscripten's port,
# which would be downloaded at build time. KugelMatch needs nothing but
# SDL2 and has no data files.
#
#   nix build .#kugelmatch-wasm   the site, in result/
#   nix run .#kugelmatch-wasm     serve it locally and open a browser

{ pkgs
, sdlSrc
, sdlVersion
, version
, gitRev
, date
}:

let
  emscripten = pkgs.emscripten;

  # Emscripten writes to its cache; start from the prebuilt one in the
  # store (libc, libc++ ...) instead of building those again.
  setupEmCache = name: ''
    export EM_CACHE="$TMPDIR/emcache-${name}"
    cp -r ${emscripten}/share/emscripten/cache "$EM_CACHE"
    chmod -R u+w "$EM_CACHE"
  '';

  # Static SDL2 for wasm32.
  sdl2Wasm = pkgs.stdenv.mkDerivation {
    pname = "sdl2-wasm";
    version = sdlVersion;
    dontUnpack = true;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.python3 ];
    env.SDL_SRC = "${sdlSrc}";
    buildPhase = ''
      runHook preBuild
      ${setupEmCache "sdl2"}
      PREFIX=$PWD/prefix bash ${../mk/wasm/scripts/build-sdl2.sh}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out
      cp -a prefix/. $out/
      runHook postInstall
    '';
    dontStrip = true; # the host's strip does not know wasm objects
    meta = {
      description = "Static SDL2 for wasm32-emscripten";
      license = pkgs.lib.licenses.zlib;
      platforms = pkgs.lib.platforms.linux;
    };
  };

  # The game's web page.
  kugelmatchWasm = pkgs.stdenv.mkDerivation {
    pname = "kugelmatch-wasm";
    inherit version;
    src = pkgs.lib.cleanSource ../.;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ emscripten pkgs.cmake pkgs.python3 ];
    buildPhase = ''
      runHook preBuild
      ${setupEmCache "kugelmatch"}
      emcmake cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$out \
        -DSDL2_ROOT=${sdl2Wasm} \
        -DPROJECT_VERSION_FULL=${version} \
        -DPROJECT_DATE=${date} \
        -DKUGELMATCH_GIT_REV=${gitRev} \
        -DKUGELMATCH_OPENGLES2=ON
      cmake --build build --parallel ''${NIX_BUILD_CORES:-1}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      cmake --install build
      runHook postInstall
    '';
    dontStrip = true;
    meta = {
      description = "KugelMatch as a web page (WebAssembly)";
      license = pkgs.lib.licenses.gpl3Plus;
      platforms = pkgs.lib.platforms.linux;
    };
  };

  # `nix run`: serve the site on 127.0.0.1 and open it in a browser.
  serveApp = {
    type = "app";
    program = toString (pkgs.writeShellScript "kugelmatch-wasm-serve" ''
      export PKG=${kugelmatchWasm}
      export APP_NAME=kugelmatch
      export PATH=${pkgs.python3}/bin:${pkgs.xdg-utils}/bin:$PATH
      exec bash ${../mk/wasm/scripts/serve.sh} "$@"
    '');
    meta.description = "Serve the WebAssembly build of KugelMatch and open it in a browser";
  };

in {
  inherit sdl2Wasm kugelmatchWasm serveApp;
}
