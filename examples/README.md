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

## Fonts

[`fallback-font-faces.json`](fallback-font-faces.json) lists Noto fonts for the
scripts that a style's glyphs can't shape, such as Arabic, Devanagari, and
Myanmar. It is the value of `mln_map_options.fallback_font_faces`, so an example
passes it to the map as it is. A map loads a font only when a label needs it.
