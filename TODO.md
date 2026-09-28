# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `da45eb7` — fix GLSL reserved word `half`

## Done

- [x] Dual CPU/GPU raytracer backends
- [x] Audio, shake, polish, resize/fullscreen
- [x] Fix GLSL compile: rename `half` → `halfExtent`

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-004.1-resize-fullscreen-a7bcc21.bundle`
Next: `kugelmatch-005.1-fix-glsl-half-a7bcc21.bundle`

## Notes for next agent

- Avoid GLSL reserved names (`half`, `fixed`, `input`, etc.) in shaders.
- Always regenerate `src/embedded_frag.inc` when editing `shaders/raytrace.frag`.
- After a new tip bundle, delete superseded intermediate bundles for this project.
