// Snippets for the docs site's Render images page. They are type-checked with
// the tests, which never run them.

import {
  CameraUpdateMode,
  GraphicsDeviceHandle,
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

export async function images(
  runtime: RuntimeHandle,
  z: number,
  x: number,
  y: number,
): Promise<void> {
  // #region map
  const extent = { width: 512, height: 512, scaleFactor: 1 };
  const { map, ready: mapReady } = runtime.createMap({
    initialExtent: extent,
    mapMode: MapMode.tile,
  });
  await mapReady;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json");
  // #endregion style

  // #region attach
  const device = GraphicsDeviceHandle.create();

  const { session, ready: sessionReady } = map.attachGraphicsDeviceTexture(
    device,
    { extent },
    // The library renders on its own thread.
    { driver: RenderDriverKind.coreWorker },
  );
  await sessionReady;
  // #endregion attach

  // #region image
  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: { center: tileCenter(z, x, y), zoom: z },
  });

  await map.requestStillImage();
  // #endregion image

  // #region read
  const image = await session.readTexture();
  copyPixels(image.data, image.info.stride);
  // #endregion read

  // #region close
  session.dispose();
  device.close();
  await map.close();
  // #endregion close
}
