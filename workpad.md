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
- Metal texture modes rendered vertically mirrored after the compositor stopped
  flipping UVs. The example now keeps the compositor as a straight copy and
  requests MapLibre's flipped-Y viewport transform for Metal texture targets.
- The shader compilation task failed for non-Vulkan variants before the OpenGL
  build could run. The task now performs work only for Vulkan without exiting
  the shell early.
- Setup and camera failure logs printed only Zig error names. Map state now owns
  a stable diagnostic store and setup/camera paths include native status details
  when the C API reports them.
- Render-session resize/render-update diagnostics were dropped at the
  render-target wrapper. The wrapper now accepts the map state's diagnostic
  store for native failure reporting.
- The specification did not define the invalid-argument exit code and mixed
  early backend validation with later startup reporting. It now specifies exit
  `1` for invalid arguments and separates validation from startup information
  printing.

## Deferred/needs follow-up

- No open deviations are currently recorded.
