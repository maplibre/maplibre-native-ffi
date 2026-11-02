export function emptyStyle(): object {
  return { version: 8, sources: {}, layers: [] };
}

export function backgroundStyle(color = "#ff0000"): object {
  return {
    version: 8,
    sources: {},
    layers: [
      {
        id: "background",
        type: "background",
        paint: { "background-color": color },
      },
    ],
  };
}
