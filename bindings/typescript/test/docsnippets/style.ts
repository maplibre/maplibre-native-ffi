// Snippets for the docs site's Style page. They are type-checked with the
// tests, which never run them.

import {
  type MapHandle,
  type PremultipliedRgba8Image,
  type RuntimeHandle,
} from "../../src/index.ts";

function drawImage(_imageId: string): PremultipliedRgba8Image {
  return {
    width: 1,
    height: 1,
    stride: 4,
    pixels: new Uint8Array([255, 0, 0, 255]),
  };
}

// pixels holds 16 x 16 premultiplied RGBA pixels.
export async function style(
  map: MapHandle,
  pixels: Uint8Array,
): Promise<void> {
  // #region load
  await map.setStyleUrl("https://demotiles.maplibre.org/style.json");
  // #endregion load

  // #region add
  map.discarding.addStyleSourceJson("satellite", {
    type: "raster",
    tileSize: 256,
    tiles: ["https://tiles.example.com/{z}/{x}/{y}.png"],
  });

  // Draw the layer under the "countries-label" layer.
  map.discarding.addStyleLayerJson(
    { id: "satellite", type: "raster", source: "satellite" },
    "countries-label",
  );
  // #endregion add

  // #region geojson
  map.discarding.addGeoJsonSource("points");
  map.discarding.setGeoJsonSourceData("points", {
    type: "Point",
    coordinates: [-122.4, 37.8],
  });
  // #endregion geojson

  // #region set-property
  // Show both changes in the same frame.
  map.discarding.beginCommandGroup();
  map.discarding.setStyleLayerProperty(
    "satellite",
    "visibility",
    "none",
  );
  map.discarding.setStyleLayerProperty(
    "countries-boundary",
    "line-color",
    ["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"],
  );
  map.discarding.endCommandGroup();
  // #endregion set-property

  // #region image
  map.discarding.setStyleImage("marker", {
    width: 16,
    height: 16,
    stride: 16 * 4,
    pixels,
  });

  // An empty layer ID draws the layer on top.
  map.discarding.addStyleLayerJson({
    id: "markers",
    type: "symbol",
    source: "points",
    layout: { "icon-image": "marker" },
  });
  // #endregion image
}

export function styleEvents(
  runtime: RuntimeHandle,
  map: MapHandle,
): void {
  // #region missing-image
  // The handler runs on this thread. The binding queues each call.
  runtime.discarding.setEventHandler({
    callback: (events) => {
      for (const { source, payload } of events) {
        if (source !== map.id || payload.kind !== "styleImage")
          continue;
        const { imageId } = payload.value;
        map.discarding.setStyleImage(imageId, drawImage(imageId));
      }
    },
  });
  // #endregion missing-image
}
