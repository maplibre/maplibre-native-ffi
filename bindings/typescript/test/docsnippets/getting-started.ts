// Snippets for the docs site's Getting started page. They are type-checked
// with the tests, which never run them.

import {
  GraphicsDeviceHandle,
  MapMode,
  RenderDriverKind,
  RuntimeHandle,
} from "../../src/index.ts";

function copyPixels(_data: Uint8Array, _stride: number): void {}

export async function gettingStarted(): Promise<void> {
  // #region runtime
  const runtime = await RuntimeHandle.create();
  // #endregion runtime

  // #region map
  const extent = { width: 512, height: 512, scaleFactor: 1 };
  const { map, ready: mapReady } = runtime.createMap({
    initialExtent: extent,
    mapMode: MapMode.staticValue,
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

  // #region render
  await map.requestStillImage();
  // #endregion render

  // #region render
  const image = await session.readTexture();
  // #endregion render

  // #region render
  copyPixels(image.data, image.info.stride);
  // #endregion render

  // #region close
  session.dispose();
  device.close();
  await map.close();
  await runtime.close();
  // #endregion close
}
