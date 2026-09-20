# Location indicator plugin

A MapLibre Native layer plugin, written in Zig, that registers the
`location-puck` style layer type through the C plugin ABI
(`mln/plugin/plugin_api.h`). A `location-puck` layer draws a procedural device
location indicator — puck, white border, bearing arrow, bearing-accuracy sector,
accuracy circle, shadow, and pulse ring — at one geographic position. All shapes
are drawn analytically in the fragment shaders, so the layer needs no sprite
images.

The layer takes its geographic position from a paint property and requires no
style source. It supports OpenGL, Vulkan, and Metal. WebGPU has no plugin shader
path.

## Driving the indicator

Set `position` to the indicator's `[latitude, longitude]`, `bearing` to the
heading in degrees clockwise from north, and `accuracy-radius` to the horizontal
accuracy in meters. The accuracy circle follows the ground plane at the
position's latitude. The components follow the ground plane and use core’s
perspective compensation (default 0.85). Tilt displacement moves the puck and
shadow in opposite directions while the bearing arrow stays at ground level. A
typical layer:

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
the style's transition duration. Bearings follow core’s shortest-arc
transitions: 350 to 10 degrees animates through north. Use continuous longitudes
when animating across the antimeridian.

Set `bearing-visible` to 1 when the caller has a heading; the default 0 hides
the arrow and the bearing-accuracy sector, because a bearing accuracy without a
bearing is meaningless.

## Paint properties

Sizes are logical pixels unless noted. Floats accept zero or more unless a
minimum or maximum is listed; a zero radius hides the component.

| Property                   | Type     | Default              | Description                                      |
| -------------------------- | -------- | -------------------- | ------------------------------------------------ |
| `position`                 | double2  | `[0, 0]`             | Indicator `[latitude, longitude]`                |
| `bearing`                  | rotation | 0                    | Heading, degrees clockwise from north            |
| `perspective-compensation` | float    | 0.85, max 1          | Core’s perspective size compensation             |
| `tilt-displacement`        | float    | 0                    | Opposite top and shadow displacement under pitch |
| `bearing-radius`           | float    | 18                   | Arrow size, independent of puck size             |
| `bearing-visible`          | float    | 0, max 1             | Shows the arrow and sector when 1                |
| `accuracy-radius`          | float    | 0                    | Accuracy circle radius, meters                   |
| `accuracy-border-width`    | float    | 0                    | Accuracy circle border width                     |
| `bearing-accuracy`         | float    | 0, max 180           | Sector half-width, degrees                       |
| `bearing-accuracy-radius`  | float    | 64                   | Sector radius                                    |
| `shadow-radius`            | float    | 0                    | Shadow radius                                    |
| `puck-radius`              | float    | 8                    | Puck fill radius                                 |
| `puck-border-width`        | float    | 2                    | Puck border width                                |
| `pulse-radius`             | float    | 0                    | Maximum pulse ring radius                        |
| `pulse-period`             | float    | 1.5, min 0.1         | Pulse period, seconds                            |
| `puck-color`               | color    | `(0.17,0.54,0.94,1)` | Puck fill                                        |
| `puck-border-color`        | color    | white                | Puck border                                      |
| `accuracy-color`           | color    | blue, 0.15 alpha     | Accuracy circle fill                             |
| `accuracy-border-color`    | color    | blue, 0.4 alpha      | Accuracy circle border                           |
| `bearing-accuracy-color`   | color    | blue, 0.3 alpha      | Sector fill; fades at the edges                  |
| `bearing-arrow-color`      | color    | white                | Arrow fill                                       |
| `shadow-color`             | color    | black, 0.25 alpha    | Shadow                                           |
| `pulse-color`              | color    | blue, 0.5 alpha      | Pulse ring; fades out as it expands              |

Every property accepts camera expressions and transitions; none accepts
data-driven expressions, which source-free layers cannot declare.

The arrow, puck, and shadow have independent sizes, corresponding to core’s
bearing, top, and shadow images. The arrow apex is `bearing-radius` pixels from
its center before perspective compensation.

The components composite bottom to top: accuracy circle, bearing-accuracy
sector, shadow, pulse ring, bearing arrow, puck.

## Integration

Register the plugin before loading a style that uses `location-puck`. The
library exports `mln_location_puck_register`, which accepts the host's register
function and a diagnostic buffer. The FFI's plugin loader supplies that
function; the plugin binary has no link dependency on the host library.

Registration and the loaded library last for the process lifetime. Build the
plugin against the same patched Native headers as the host. Source-free plugin
layers return a GeoJSON Point at `[longitude, latitude]` for hits on the arrow
or puck envelopes. As in core, shadow, accuracy, and bearing-accuracy geometry
do not add hit areas. The pulse does not add one either.

A positive `pulse-radius` requests continuous repaint through the normal render
session contract. The pulse uses the host's frame timestamp; still images
capture that timestamp's phase.

## Build and validation

```bash
mise run //plugins/location-indicator:build
mise run //plugins/location-indicator:test
mise run //examples/zig-readback:run
mise run //examples/zig-map:run owned-texture
```

The build installs `maplibre-location-puck` under `zig-out/lib` with the
platform's shared-library suffix. The geometry tests cover coordinate precision,
accuracy scaling, pulse timing, and hidden components. The readback example
loads the shared library, renders the indicator, and writes
`examples/zig-readback/zig-out/zig-readback.ppm`.
