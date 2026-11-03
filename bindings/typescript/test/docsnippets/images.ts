// Snippets for the docs site's Render images page. They are type-checked with
// the tests, which never run them.

import {
  CameraUpdateMode,
  type LatLng,
  MapMode,
  RenderDriverKind,
  RuntimeHandle,
} from "../../src/index.ts";

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
function tileCenter(_z: number, _x: number, _y: number): LatLng {
  return { latitude: 0, longitude: 0 };
}

function copyPixels(_data: Uint8Array, _stride: number): void {}

export async function images(runtime: RuntimeHandle): Promise<void> {
  // #region map
  const extent = { width: 800, height: 600, scaleFactor: 2 };
  const { map, ready: mapReady } = runtime.createMap({
    initialExtent: extent,
    mapMode: MapMode.static,
  });
  await mapReady;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    "https://demotiles.maplibre.org/style.json",
  );
  // #endregion style

  // #region attach
  const { session, ready: sessionReady } = map.attachOffscreen(
    { extent },
    // The library renders on its own thread.
    { driver: RenderDriverKind.coreWorker },
  );
  await sessionReady;
  // #endregion attach

  // #region image
  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: {
      center: { latitude: 48.8566, longitude: 2.3522 },
      zoom: 4,
    },
  });

  await map.requestStillImage();
  // #endregion image

  // #region read
  const image = await session.readTexture();
  copyPixels(image.data, image.info.stride);
  // #endregion read

  session.dispose();
  await map.close();
}

export async function imagesTile(
  runtime: RuntimeHandle,
  z: number,
  x: number,
  y: number,
): Promise<void> {
  // #region tile
  const { map, ready } = runtime.createMap({
    initialExtent: { width: 512, height: 512, scaleFactor: 1 },
    mapMode: MapMode.tile,
  });
  await ready;

  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    // A 512-pixel map shows tile z/x/y at zoom z.
    camera: { center: tileCenter(z, x, y), zoom: z },
  });
  // #endregion tile

  await map.close();
}
