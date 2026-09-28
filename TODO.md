# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `15f8a39` — iterative GLSL trace (no recursion)

## Done

- [x] Dual backends, audio, polish, resize/fullscreen
- [x] GLSL fixes: reserved `half`, no recursive `shade`
- [x] Wall decos, swapped left/right controls

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-007.1-swap-controls-a7bcc21.bundle`
Next tip bundle: `kugelmatch-008.1-fix-glsl-recursion-a7bcc21.bundle`

## Notes for next agent

- GLSL must not use recursion or reserved names (`half`, etc.).
- Always regenerate `src/embedded_frag.inc` when editing `shaders/raytrace.frag`.
- After a new tip bundle, delete superseded intermediate bundles for this project.
