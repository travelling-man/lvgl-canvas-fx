# lvgl-canvas-fx

An ESPHome external component that renders fun, high‑performance visual effects on an **LVGL Canvas**. It’s designed for ESP32‑class devices (ESPHome 2026.4.0+, LVGL 9.5) and works with any LVGL‑backed display (HUB75 matrices, TFTs, etc.).

> **About this fork:** This is a fork of [pavlov-net/lvgl-canvas-fx](https://github.com/pavlov-net/lvgl-canvas-fx) by Stuart Parmenter. It adds three effects designed for small LED matrices (tested on a 64×32 HUB75 panel with an Adafruit MatrixPortal S3): `galaxy`, `christmas_tree` and `fairy_lights`. Everything else is unchanged from upstream.

> **Status:** early but functional. Effects today: `circle`, `fireworks`, `fireplace`, `aurora`, `audio_spectrum`, `plasma`, `breathing`, `clouds`, `sunset`, `starfield`, `snowfall`, `leaves`, `sakura`, `fireflies`, `warp`, `boids`, `ripples`, `matrix`, `sand`, `lightning`, `lava_lamp`, `rain`, plus in this fork `galaxy`, `christmas_tree`, `fairy_lights`. Contributions welcome!

---

## Features
- **Drop‑in ESPHome component**: simple YAML config; no app code required.
- **Multiple effects** with easy runtime switching via `lvgl_canvas_fx.set_effect`.
- **Tunable FPS**: With an ESP32-S3, currently on 64×64 you can reach ~60 FPS, on 128×64 closer to 30 FPS, and on 800×480 around 7 FPS. Depends a good bit on your display.
- **Pausing/resuming** to save cycles when a page is hidden.
- **Physics‑based fireworks** powered by Chipmunk2D.

## Requirements
- **ESPHome:** 2026.4.0 or newer (bundles LVGL 9.5)
- **LVGL:** 9.5 (ESPHome's built‑in LVGL integration)

## Quick start (full example)
The example below wires up a Canvas, the effect engine, and a template `select` so you can switch effects at runtime. It also pauses the effect when the page is not visible.

```yaml
substitutions:
  DISPLAY_W: "128"
  DISPLAY_H: "64"

external_components:
  - source: github://travelling-man/lvgl-canvas-fx@main
    components: [lvgl_canvas_fx]

select:
  - platform: template
    id: fx_effect_select
    name: "FX Effect"
    optimistic: true
    options:
      - circle
      - fireworks
      - fireplace
      - aurora
      - plasma
      - breathing
      - clouds
      - sunset
      - starfield
      - snowfall
      - leaves
      - sakura
      - fireflies
      - warp
      - boids
      - ripples
      - matrix
      - sand
      - lightning
      - lava_lamp
      - rain
      - galaxy
      - christmas_tree
      - fairy_lights
    initial_option: fireworks
    set_action:
      - lvgl_canvas_fx.set_effect:
          id: cfx
          effect: !lambda 'return x;'

lvgl_canvas_fx:
  - id: cfx
    effect: fireworks  # or circle or fireplace
    canvas: fx_canvas
    fps: 24
    start_paused: true

lvgl:
  pages:
    - id: fx_page
      widgets:
        - canvas:
            id: fx_canvas
            width: ${DISPLAY_W}
            height: ${DISPLAY_H}
      on_load:
        then:
          # Keep the selected option authoritative: sync effect to current select value
          - lvgl_canvas_fx.set_effect:
              id: cfx
              effect: !lambda 'return id(fx_effect_select).state;'
          - lvgl_canvas_fx.resume:
              id: cfx
      on_unload:
        then:
          - lvgl_canvas_fx.pause:
              id: cfx
```

### Minimal example (no select)
```yaml
lvgl_canvas_fx:
  - id: cfx
    effect: fireplace
    canvas: fx_canvas
    fps: 24

lvgl:
  pages:
    - id: fx_page
      widgets:
        - canvas:
            id: fx_canvas
            width: ${DISPLAY_W}
            height: ${DISPLAY_H}
```

## Options
Under the `lvgl_canvas_fx:` array you can configure:

| Option          | Type     | Default  | Notes |
|-----------------|----------|----------|------|
| `id`            | ID       | —        | Required. Instance id.
| `canvas`        | string   | —        | Required. The LVGL Canvas widget id to render into.
| `effect`        | string   | —        | Required. Any effect key from the table below.
| `fps`           | float    | `30`     | Target frame rate (1 to 240). Lower to save CPU.
| `start_paused`  | bool     | `false`  | Start paused and `resume` when visible.

### Effects
| Key | Description |
|-----|-------------|
| `circle` | Simple bouncing circle (demo/reference effect). |
| `fireworks` | Physics-based fireworks (Chipmunk2D) with many burst shapes. |
| `fireplace` | Classic heat-field fire with embers. |
| `aurora` | Northern-lights color field. |
| `audio_spectrum` | FFT spectrum bars; feed audio via `submit_data()`. |
| `plasma` | Slow demoscene plasma through a muted cyan/violet palette. |
| `breathing` | Soft radial glow pulsing on a calm ~6 s cycle — a breathing pace aid. |
| `clouds` | Drifting fog banks, low-contrast gray-blue. |
| `sunset` | Vertical sky gradient cycling night→dawn→day→dusk. HA-controllable (see below). |
| `starfield` | Twinkling stars with an occasional shooting star. |
| `snowfall` | Two-layer parallax snow with sine sway. |
| `leaves` | Tumbling autumn leaves. |
| `sakura` | Same engine as `leaves`, cherry-blossom palette. |
| `fireflies` | Warm wandering dots, each breathing on its own rhythm. |
| `warp` | Hyperspace starfield streaking out from center. |
| `boids` | A tiny flock (murmuration) with faint trails. |
| `ripples` | Expanding raindrop rings on still water. |
| `matrix` | Ambient smoothed green digital rain — muted, no strobing. |
| `sand` | Falling-sand toy with slowly shifting pastel grains. |
| `lightning` | Dark storm sky with rare branching bolts and a decaying flash. |
| `lava_lamp` | Metaball blobs drifting and merging. |
| `rain` | Rain on glass: streaks plus clinging droplets that slide and merge. HA-controllable (see below). |
| `galaxy` | *(fork)* Tilted four-armed spiral galaxy slowly rotating around its bright core, with twinkling stars in the arms. |
| `christmas_tree` | *(fork)* Tiered tree with independently twinkling baubles, a pulsing star, snowfall and a snowy ground line. |
| `fairy_lights` | *(fork)* A string of bulbs hanging in loops from the top edge; the glow runs along the string, with light snow behind. |

The fork effects are tuned for small, low‑resolution panels such as 64×32 and adapt to the canvas size. Their look can be tweaked via the constants in the `tuning` block of each header (`fx_galaxy.h`, `fx_christmas_tree.h`, `fx_fairy_lights.h`): rotation speed, number of arms, baubles, bulbs per loop and so on.

### Services (actions)
- `lvgl_canvas_fx.set_effect: { id, effect }`
- `lvgl_canvas_fx.pause: { id }`
- `lvgl_canvas_fx.resume: { id }`
- `lvgl_canvas_fx.toggle: { id }`
- `lvgl_canvas_fx.set_fps: { id, fps }`

### Effect parameters (`submit_data`)
Some effects accept runtime data through the instance's `submit_data(const void*, size_t)` method (also how `audio_spectrum` receives audio). `rain` and `sunset` take a single **float 0..1**:

- `rain` — intensity: `0.0` = no rain (screen drains to black), `1.0` = downpour. Perfect for mirroring real weather from Home Assistant.
- `sunset` — sky phase: `0.0` = midnight, `0.25` = dawn, `0.5` = midday, `0.75` = dusk. Map your sun elevation or time of day; without data it free-runs a full cycle every ~4 minutes.

```yaml
number:
  - platform: template
    name: "Rain Intensity"
    min_value: 0
    max_value: 100
    step: 5
    optimistic: true
    restore_value: true
    set_action:
      - lambda: |-
          float v = x / 100.0f;
          id(cfx)->submit_data(&v, sizeof(v));
```

Caveats: `submit_data` reaches only the *currently active* effect, so re-send the value after `set_effect` switches to it; values sent before the effect is bound are safely dropped.

## Tips & troubleshooting
- **Page lifecycle:** Use `start_paused: true` and call `resume` on `on_load` and `pause` on `on_unload` to avoid burning cycles off‑screen.
- **Canvas buffer:** The ESPHome LVGL Canvas widget creates its buffer—no need to create/free one in the component.
- **Performance:** Rendering cost depends heavily on canvas resolution. At small sizes (64×64) 60 FPS is achievable; at 128×64 you may see ~30 FPS; at 800×480 expect ~7 FPS.
- **Memory usage:** Some effects are memory‑heavy. Small resolutions may run without PSRAM, but higher resolutions require PSRAM enabled.

## Roadmap
- More effects (PRs welcome!)
- Color themes & parameters per effect
- Metrics counters & debug overlay

## Development
Directory layout mirrors ESPHome external component conventions:
```
/esphome/components/lvgl_canvas_fx/
  lvgl_canvas_fx.py         # component schema & codegen
  lvgl_canvas_fx.cpp/.h     # runtime
  fx_*.cpp/.h               # individual effects
  register_builtin_effects.cpp
```

### Building locally
Point ESPHome at your local checkout:
```yaml
external_components:
  - source:
      type: local
      path: "/path/to/lvgl-canvas-fx/esphome/components"
    components: [lvgl_canvas_fx]
```

## License
MIT © Stuart Parmenter. The effects added in this fork (`galaxy`, `christmas_tree`, `fairy_lights`) are released under the same MIT license.
