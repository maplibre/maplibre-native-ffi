// Snippets for the docs site's Query page. They are type-checked with the
// tests, which never run them.

import type { RenderSessionHandle } from "../../src/index.ts";

function useFeature(_feature: unknown): void {}

export async function query(
  session: RenderSessionHandle,
  point: { x: number; y: number },
): Promise<void> {
  // #region rendered
  const hits = await session.queryRenderedFeatures(
    {
      kind: "box",
      value: {
        min: { x: point.x - 5, y: point.y - 5 },
        max: { x: point.x + 5, y: point.y + 5 },
      },
    },
    { layerIds: ["poi"] },
  );
  for (const hit of hits) {
    // hit.feature is the parsed GeoJSON feature.
    useFeature(hit.feature);
  }
  // #endregion rendered

  // #region source
  const features = await session.querySourceFeatures("openmaptiles", {
    sourceLayerIds: ["transportation"],
    // A filter from the MapLibre style spec.
    filter: ["==", ["get", "class"], "motorway"],
  });
  for (const feature of features) {
    useFeature(feature.feature);
  }
  // #endregion source
}
