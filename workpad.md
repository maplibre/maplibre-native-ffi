# zig-map map-example specification audit

## Resolved deviations

- CLI accepted `--render-target`, `--render-target=...`, `-h`, and an implicit
  `owned-texture` default. The specification requires one positional mode value
  and permits only `--help`.
- Startup logging printed the mode and native backend support before the map and
  render session were attached, and did not print the required exact render
  target status line.
- Runtime event draining only reacted to `map_render_update_available`.
  Continuous mode also requires repaint when this map receives
  `map_render_frame_finished` with `needs_repaint`.
- The printed controls and keyboard handling included PageUp/PageDown aliases
  outside the specified control scheme.
- The Metal compositor used two triangles and flipped UVs. The specification
  requires a fullscreen triangle and a straight texture copy with standard UV
  orientation.
- The shader compilation task failed for non-Vulkan variants before the OpenGL
  build could run. The task now performs work only for Vulkan without exiting
  the shell early.
- Setup and camera failure logs printed only Zig error names. Map state now owns
  a stable diagnostic store and setup/camera paths include native status details
  when the C API reports them.

## Deferred/needs follow-up

- No open deviations are currently recorded.
