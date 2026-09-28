# TODO / Handoff

## Current tip

Base of this work line: `a7bcc21` (Initial checkin)

**Tip:** `863a9f8` — resize + fullscreen (F11 / Alt+Enter)

## Done

- [x] Dual CPU/GPU raytracer backends
- [x] Procedural audio, mute/volume, camera shake
- [x] Serve delay, impact flash, vignette, FPS HUD
- [x] Resizable window with letterboxed 640×480 presentation
- [x] Desktop fullscreen (F11, Alt+Enter)

## Open / follow-ups

- [ ] Optional: compute-shader path
- [ ] Optional: multi-sample soft shadows on GPU
- [ ] Score overlay in-framebuffer (bitmap font)
- [ ] Windows / macOS build notes

## Bundle naming for this stack

Base short: `a7bcc21`
Previous: `kugelmatch-003.1-polish-a7bcc21.bundle`
Next: `kugelmatch-004.1-resize-fullscreen-a7bcc21.bundle`

## Notes for next agent

- Internal resolution is always 640×480. Never scale the ray count with the window.
- GPU: FBO + `glBlitFramebuffer` letterbox in `present()`; track drawable size for HiDPI.
- CPU: `SDL_RenderSetLogicalSize(640,480)`.
- Fullscreen uses `SDL_WINDOW_FULLSCREEN_DESKTOP` (borderless), not exclusive mode.
- After a new tip bundle, delete superseded intermediate bundles for this project.
