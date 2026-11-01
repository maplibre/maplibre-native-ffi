// Snippets for the docs site's Render images page. They are analyzed with
// the tests, which never run them.

import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
LatLng tileCenter(int z, int x, int y) => const LatLng();

void copyPixels(Uint8List data, int stride) {}

Future<void> images(RuntimeHandle runtime, int z, int x, int y) async {
  // #region map
  const extent = LogicalExtent(width: 512, height: 512, scaleFactor: 1);
  final NewMap(:map, ready: mapReady) = runtime.createMap(
    const MapOptions(initialExtent: extent, mapMode: MapMode.tile),
  );
  await mapReady;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl('https://demotiles.maplibre.org/style.json');
  // #endregion style

  // #region attach
  final device = GraphicsDeviceHandle.create();

  final NewRenderSession(:session, ready: sessionReady) = map
      .attachGraphicsDeviceTexture(
        device,
        const GraphicsDeviceTextureDescriptor(extent: extent),
        // The library renders on its own thread.
        const RenderSessionAttachOptions(driver: RenderDriverKind.coreWorker),
      );
  await sessionReady;
  // #endregion attach

  // #region image
  map.discarding.updateCamera(
    CameraUpdate(
      mode: CameraUpdateMode.jump,
      camera: CameraOptions(center: tileCenter(z, x, y), zoom: z.toDouble()),
    ),
  );

  await map.requestStillImage();
  // #endregion image

  // #region read
  final image = await session.readTexture();
  copyPixels(image.data, image.info.stride);
  // #endregion read

  // #region close
  session.dispose();
  device.close();
  await map.close();
  // #endregion close
}
