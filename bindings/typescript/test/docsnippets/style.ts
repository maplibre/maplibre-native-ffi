// Snippets for the docs site's Style page. They are type-checked with the
// tests, which never run them.

import {
  GeoJsonSourceDataHandle,
  type MapHandle,
  type PremultipliedRgba8Image,
  RuntimeEventType,
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
export function style(map: MapHandle, pixels: Uint8Array): void {
  // #region load
  map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json");
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
  const data = GeoJsonSourceDataHandle.create({
    type: "Point",
    coordinates: [-122.4, 37.8],
  });
  map.discarding.addGeoJsonSourceData("points", data);
  // The source keeps its own reference to the data.
  data.close();

  // Later, prepare new data with the same options, and set it.
  const next = GeoJsonSourceDataHandle.create({
    type: "Point",
    coordinates: [2.35, 48.86],
  });
  map.discarding.setGeoJsonSourceData("points", next);
  next.close();
  // #endregion geojson

  // #region set-property
  // Show both changes in the same frame.
  map.discarding.beginCommandGroup();
  map.discarding.setStyleLayerProperty("satellite", "visibility", "none");
  map.discarding.setStyleLayerProperty("countries-boundary", "line-color", [
    "interpolate",
    ["linear"],
    ["zoom"],
    5,
    "#9cf",
    12,
    "#06c",
  ]);
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
  map.discarding.addStyleLayerJson(
    {
      id: "markers",
      type: "symbol",
      source: "points",
      layout: { "icon-image": "marker" },
    },
    "",
  );
  // #endregion image
}

export function styleEvents(runtime: RuntimeHandle, map: MapHandle): void {
  // #region missing-image
  // The handler runs on this thread. The binding queues each call.
  runtime.discarding.setEventHandler((events) => {
    for (const event of events) {
      if (
        event.source !== map.id ||
        event.type !== RuntimeEventType.mapStyleImageMissing
      ) {
        continue;
      }
      // The message holds the image's ID.
      map.discarding.setStyleImage(event.message, drawImage(event.message));
    }
  });
  // #endregion missing-image
}
