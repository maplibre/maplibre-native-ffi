# Location indicator plugin

A MapLibre Native layer plugin, written in Zig, that registers the
`location-puck` style layer type through the C plugin ABI
(`mln/plugin/plugin_api.h`). A `location-puck` layer draws a procedural device
location indicator — puck, white border, bearing arrow, bearing-accuracy sector,
accuracy circle, shadow, and pulse ring — at one geographic position. All shapes
are drawn analytically in the fragment shaders, so the layer needs no sprite
images.

The layer is source-free: it takes no source in the style. Its geometry arrives
on every rendered frame from the plugin's `build_frame` callback, which projects
the evaluated `position` to screen space. Every paint property is a constant or
camera expression, so every property — including `position` and `bearing` —
transitions smoothly over the style's transition duration.

The plugin works on the OpenGL, Vulkan, and Metal backends; each shader ships
GLSL for OpenGL and Vulkan plus a single Metal source. A WebGPU build registers
the plugin but has no shader path for its layers.

## Driving the indicator

Set `position` to the indicator's `[latitude, longitude]`, `bearing` to the
heading in degrees clockwise from north, and `accuracy-radius` to the horizontal
accuracy in meters. The host converts meters to pixels from the position's own
latitude. A typical layer:

```json
{
  "id": "puck",
  "type": "location-puck",
  "paint": {
    "position": [37.7749, -122.4194],
    "bearing": 35,
    "accuracy-radius": 80,
    "bearing-visible": 1
  }
}
```

Move, rotate, and rescale the indicator by updating the paint properties — in
the style or through the layer-property setter — and each change animates over
the style's transition duration.

Set `bearing-visible` to 1 when the caller has a heading; the default 0 hides
the arrow and the bearing-accuracy sector, because a bearing accuracy without a
bearing is meaningless.

## Paint properties

Sizes are logical pixels unless noted. Floats accept zero or more unless a
minimum or maximum is listed; a zero radius hides the component.

| Property                  | Type   | Default              | Description                             |
| ------------------------- | ------ | -------------------- | --------------------------------------- |
| `position`                | float2 | `[0, 0]`             | Indicator `[latitude, longitude]`       |
| `bearing`                 | float  | 0                    | Heading, degrees clockwise from north   |
| `bearing-visible`         | float  | 0, max 1             | Shows the arrow and sector when 1       |
| `accuracy-radius`         | float  | 0                    | Accuracy circle radius, meters          |
| `accuracy-border-width`   | float  | 0                    | Accuracy circle border width            |
| `bearing-accuracy`        | float  | 0, max 180           | Sector half-width, degrees              |
| `bearing-accuracy-radius` | float  | 64                   | Sector radius                           |
| `shadow-radius`           | float  | 0                    | Shadow ellipse radius                   |
| `puck-radius`             | float  | 8                    | Puck radius; also sizes the arrow       |
| `puck-border-width`       | float  | 2                    | Puck border width; also sizes the arrow |
| `pulse-radius`            | float  | 0                    | Maximum pulse ring radius               |
| `pulse-period`            | float  | 1.5, min 0.1         | Pulse period, seconds                   |
| `puck-color`              | color  | `(0.17,0.54,0.94,1)` | Puck fill                               |
| `puck-border-color`       | color  | white                | Puck border                             |
| `accuracy-color`          | color  | blue, 0.15 alpha     | Accuracy circle fill                    |
| `accuracy-border-color`   | color  | blue, 0.4 alpha      | Accuracy circle border                  |
| `bearing-accuracy-color`  | color  | blue, 0.3 alpha      | Sector fill; fades at the edges         |
| `bearing-arrow-color`     | color  | white                | Arrow fill                              |
| `shadow-color`            | color  | black, 0.25 alpha    | Shadow ellipse                          |
| `pulse-color`             | color  | blue, 0.5 alpha      | Pulse ring; fades out as it expands     |

Every property accepts camera expressions and transitions; none accepts
data-driven expressions, which source-free layers cannot declare.

The arrow derives from the puck: its apex sits at 1.8 times the outer puck
radius (`puck-radius` plus `puck-border-width`), matching the proportion of the
core indicator's bearing image.

The components composite bottom to top: accuracy circle, bearing-accuracy
sector, shadow ellipse, pulse ring, bearing arrow, puck.

## Limitations

- Source-free layers receive no feature hit-testing, so rendered-feature queries
  never answer for this layer. Hit areas were hit-testable in the tile-driven
  design; nothing equivalent exists here.
- The style must contain at least one source for any source-free layer to
  render, a constraint the plugin ABI inherits from MapLibre Native's render
  orchestrator.
- In continuous-mode maps the layer reports needs-repaint on every rendered
  frame while `pulse-radius` is above zero, through the plugin ABI's
  `should_animate` callback; hosts re-render through the normal needs-repaint
  and repaint-request contract. Upstream's placement controller can keep
  needs-repaint set after the callback returns zero, so treat needs-repaint as a
  hint to re-render rather than as proof that an animation is running. A static
  still image captures the pulse at an arbitrary wall-clock phase.
- The sector fades linearly from the center to its outer edge and feathers its
  angular edges by about a pixel, matching the core location indicator's
  bearing-accuracy gradient.
- The plugin must register before any style that uses `location-puck` loads;
  registration is process-wide.

## Loading

The package builds the plugin as a shared library, `maplibre-location-puck`
(`libmaplibre-location-puck.dylib`, `.so`, or `.dll` by platform). The library
exports `mln_location_puck_register`, which takes the host's register function
and the registration error buffer. The host library hands out that function
through `mln_plugin_get_register_function_v1`, so the plugin binary never links
the host, and any consumer that can open a shared library and pass a function
pointer can load it:

1. Open the shared library and look up `mln_location_puck_register`.
2. Call `mln_plugin_get_register_function_v1` on the host library.
3. Pass the register function to the plugin's entry point.

Registration is process-wide and must happen before any style that uses
`location-puck` loads. As with in-process registration, the plugin must be built
against the same MapLibre Native revision as the host library.

The zig-readback example (`examples/zig-readback`) drives exactly this path.

## Build

```bash
mise run //plugins/location-indicator:build   # writes zig-out/lib/libmaplibre-location-puck.dylib
```

For a live render, `mise run //examples/zig-readback:run` loads the library
through the Zig binding's `loadPlugin` and draws the indicator at the demo map's
center, then checks the center pixel before writing `zig-out/zig-readback.ppm`.
