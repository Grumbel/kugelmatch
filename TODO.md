# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `47f8799` — swap left/right controls

## Done

- [x] Dual backends, audio, polish, resize/fullscreen, GLSL half fix
- [x] Decorative spheres moved to side/back walls (7 orbs + ball)

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-005.1-fix-glsl-half-a7bcc21.bundle`
Next: `kugelmatch-006.1-wall-decos-a7bcc21.bundle`

## Notes for next agent

- GPU `MAX_SPHERES` is 8 (1 ball + 7 decos). Raise shader + C++ limits together if adding more.
- Deco spheres are visual only; physics only tracks the match ball.
- After a new tip bundle, delete superseded intermediate bundles for this project.
