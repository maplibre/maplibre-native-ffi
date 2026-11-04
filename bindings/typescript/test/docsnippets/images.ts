// Snippets for the docs site's Render images page. They are type-checked with
// the tests, which never run them.

import {
  CameraUpdateMode,
  cameraForTile,
  MapMode,
  RenderDriverKind,
  RuntimeHandle,
} from "../../src/index.ts";

function copyPixels(_data: Uint8Array, _stride: number): void {}

export async function images(runtime: RuntimeHandle): Promise<void> {
  // #region map
  const map = runtime.createMap({
    initialExtent: { width: 800, height: 600, scaleFactor: 2 },
    mapMode: MapMode.static,
  });
  await map.ready;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    "https://demotiles.maplibre.org/style.json",
  );
  // #endregion style

  // #region attach
  const session = map.attachOffscreen(
    { logicalSize: { width: 800, height: 600 } },
    // The library renders on its own thread.
    { driver: RenderDriverKind.coreWorker },
  );
  await session.ready;
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
  await map.release();
}

export async function imagesTile(
  runtime: RuntimeHandle,
  z: number,
  x: number,
  y: number,
): Promise<void> {
  // #region tile
  const map = runtime.createMap({
    initialExtent: { width: 512, height: 512, scaleFactor: 1 },
    mapMode: MapMode.tile,
  });
  await map.ready;

  map.discarding.updateCamera({
    mode: CameraUpdateMode.jump,
    camera: cameraForTile({ z, x, y }, { width: 512, height: 512 }),
  });
  // #endregion tile

  await map.release();
}
