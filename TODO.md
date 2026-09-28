# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip after this session:** see latest commit on `master` (will be recorded after bundle creation).

## Done this session

- [x] Added AGENTS.md / TODO.md
- [x] Extracted shared `Scene` + math into clean headers
- [x] Cleaned CPU raytracer (raytracer.cpp) — removed dead paths, consistent naming
- [x] Added GPU backend: OpenGL 3.3 core, fullscreen triangle, full raytracing in fragment shader
- [x] Single binary with `--cpu` / `--gpu` (default CPU)
- [x] Updated flake.nix + CMake for OpenGL / GLEW
- [x] README updated
- [x] SPDX / GPLv3+ headers on new and touched sources

## Open / follow-ups

- [ ] Optional: compute-shader path (same algorithm, better for higher res)
- [ ] Optional: more materials / soft shadows with multiple samples on GPU
- [ ] Score overlay in-framebuffer (currently window title only)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`  
Next bundle: `kugelmatch-NNN.M-…-a7bcc21.bundle` (start at 001.1)

## Notes for next agent

- GPU shader embeds scene limits (MAX_SPHERES etc.); keep C++ upload arrays in sync.
- Do not introduce triangle meshes of the playfield — both backends must remain pure raytracers.
- After producing a new tip bundle, delete superseded intermediate bundles for this project.
