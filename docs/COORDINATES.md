<!--
SPDX-License-Identifier: GPL-3.0-or-later
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-->

# Coordinate system

KugelMatch uses a **right-handed** world frame.

```
        +Y (up, toward ceiling)
         |
         |
         o------ +X  (screen-right when the paddle camera looks down the court)
        /
       /
     +Z  (toward the far paddle / AI end)
```

| Axis | Meaning |
|------|---------|
| **+X** | Toward the right wall from the near (player) end looking down-field |
| **+Y** | Up (floor at `y = 0`, ceiling plane at `y = WALL_H`) |
| **+Z** | Toward the far end of the court (`z ≈ 0` near player, `z ≈ FIELD_L` far paddle) |

## Court layout

| Object | Approx. position |
|--------|------------------|
| Near paddle (P1) | `z ≈ 0.25`, moves along **X** |
| Far paddle (P2 / AI) | `z ≈ FIELD_L − 0.25`, moves along **X** |
| Ball | `y ≈ BALL_R`, moves in the **X–Z** plane |
| Scoreboard | Midfield `z = FIELD_L/2`, high under the ceiling; digits face **−Z** (toward P1) |
| Paddle camera | Near `z ≈ −0.6`, looks toward **+Z** |

Field size constants (`include/game.hpp`): `FIELD_W` (X span), `FIELD_L` (Z span), `WALL_H` (ceiling height).

## Camera basis

`Camera::set(position, lookAt, worldUp, fovDeg)` builds:

```text
forward = normalize(lookAt − position)
right   = normalize(worldUp × forward)   // screen +u → +right
up      = normalize(forward × right)     // screen +v → +up
```

Primary rays (CPU and GPU):

```text
dir = normalize(forward + right * u + up * v)
```

- **u** increases toward the **right** edge of the image.
- **v** increases toward the **top** of the image (CPU: framebuffer row 0 is top; GPU: OpenGL `v_uv.y = 1` is top).

With the paddle camera looking down **+Z**, screen-right is **+X** and screen-up is **+Y**.

## Controls vs axes

Looking down **+Z**, **A** / **←** move the paddle toward **−X** (screen-left), **D** / **→** toward **+X** (screen-right).

## Historical note

An earlier basis used `up = right × forward` together with `right = worldUp × forward`, which made `up = −Y` and drew the whole scene upside-down. That was corrected so `up = forward × right`.
