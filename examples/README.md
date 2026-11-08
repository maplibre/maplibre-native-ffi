# Examples

Each directory holds an app that shows a map with one binding. Its README says
how to run it.

## Styles

[`styles.json`](styles.json) lists the map styles that the examples offer, so
every example can show the same set. The browser example reads it; the others
still load only `default`. Each style has:

- `id`: a stable name, which the browser example puts in its URL.
- `name` and `provider`: what a picker shows.
- `url`, or `json` with the style itself when no server hosts it. Load the first
  with `mln_map_set_style_url()` and the second with `mln_map_set_style_json()`.
- `dark`: whether the style draws a dark map, so an app can match its controls.
- `swatch`: the style's land, water, and road colors, for a preview.

Add a style only if every file it loads, such as its TileJSON, tiles, glyphs,
and sprites, allows cross-origin requests, since the browser example runs on
another origin.
