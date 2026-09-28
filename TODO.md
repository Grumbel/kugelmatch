# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `ebd5918` — Wire dual backends, CLI, and packaging

## Done this session

- [x] Added AGENTS.md / TODO.md / COPYING / .gitignore
- [x] Extracted shared Scene + Camera into scene.hpp
- [x] Cleaned CPU raytracer (CpuRaytracer)
- [x] Added GPU backend: OpenGL 3.3 core, fullscreen triangle, full raytracing in fragment shader (no scene meshes)
- [x] Single binary with `--cpu` / `--gpu` (default CPU)
- [x] Updated flake.nix + CMake (pkg-config SDL2, OpenGL)
- [x] README updated
- [x] SPDX / GPLv3+ headers on new and touched sources

## Open / follow-ups

- [ ] Optional: compute-shader path (same algorithm, better for higher res)
- [ ] Optional: more materials / soft shadows with multiple samples on GPU
- [ ] Score overlay in-framebuffer (currently window title only)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Next bundle: `kugelmatch-001.1-gpu-raytracer-a7bcc21.bundle`

## Notes for next agent

- GPU shader embeds scene limits (MAX_SPHERES=8, MAX_BOXES=4, MAX_PLANES=8); keep C++ upload arrays in sync with shaders/raytrace.frag and embedded_frag.inc.
- Do not introduce triangle meshes of the playfield — both backends must remain pure raytracers.
- After producing a new tip bundle, delete superseded intermediate bundles for this project.
- Run from a directory that can see `shaders/` or rely on the embedded fallback.
