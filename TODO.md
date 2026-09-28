# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** f19c893 — metallic clanks + camera shake

## Done

- [x] Dual CPU/GPU raytracer backends
- [x] AGENTS.md / TODO.md / COPYING / packaging
- [x] Procedural metallic clank sounds on paddle/wall collisions (SDL audio)
- [x] Soft thud on scoring
- [x] Camera shake on collisions (stronger for player paddle hits)

## Open / follow-ups

- [ ] Optional: compute-shader path (same algorithm, better for higher res)
- [ ] Optional: more materials / soft shadows with multiple samples on GPU
- [ ] Score overlay in-framebuffer (currently window title only)
- [ ] Windows / macOS build notes
- [ ] Optional: mute key / volume control

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-001.1-gpu-raytracer-a7bcc21.bundle`
Next: `kugelmatch-002.1-audio-shake-a7bcc21.bundle`

## Notes for next agent

- Audio is procedural (no WAV assets); `Audio` mixes up to 8 voices in an SDL float callback.
- Camera shake is applied in `Game::run` as offset on camPos/lookAt; decay in `updateShake`.
- GPU shader limits (MAX_SPHERES etc.) must stay in sync with C++ upload.
- Both backends remain pure raytracers — no scene meshes.
- After a new tip bundle, delete superseded intermediate bundles for this project.
