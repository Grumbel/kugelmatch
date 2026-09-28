# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `ce70813` — polish (mute, serve delay, flash, vignette, FPS)

## Done

- [x] Dual CPU/GPU raytracer backends
- [x] Procedural metallic clanks + soft thud
- [x] Camera shake on collisions
- [x] Mute (M) and master volume (+/−)
- [x] Serve delay after points
- [x] Ball impact flash
- [x] Soft vignette (CPU + GPU)
- [x] FPS in window title

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous tip bundle: `kugelmatch-002.1-audio-shake-a7bcc21.bundle`
Next: `kugelmatch-003.1-polish-a7bcc21.bundle`

## Notes for next agent

- Audio mute/master are atomics; safe from the SDL callback.
- Serve uses `serveTimer_` / `queueServe`; physics paused while serving.
- Vignette lives in CPU `renderRow` and GLSL `main`; regenerate `embedded_frag.inc` if the frag shader changes.
- Both backends remain pure raytracers — no scene meshes.
- After a new tip bundle, delete superseded intermediate bundles for this project.
