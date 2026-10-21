/** A style with no sources and no layers. */
export function emptyStyle(): string {
  return JSON.stringify({ version: 8, sources: {}, layers: [] });
}

/** A style with one background layer of a solid color. */
export function backgroundStyle(color = "#ff0000"): string {
  return JSON.stringify({
    version: 8,
    sources: {},
    layers: [
      {
        id: "background",
        type: "background",
        paint: { "background-color": color },
      },
    ],
  });
}
