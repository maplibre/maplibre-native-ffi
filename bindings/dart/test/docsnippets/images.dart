// Snippets for the docs site's Render images page. They are analyzed with
// the tests, which never run them.

import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void copyPixels(Uint8List data, int stride) {}

Future<void> images(RuntimeHandle runtime) async {
  // #region map
  final map = runtime.createMap(
    const MapOptions(
      initialExtent: LogicalExtent(
        width: 800,
        height: 600,
        scaleFactor: 2,
      ),
      mapMode: MapMode.static,
    ),
  );
  await map.ready;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    'https://demotiles.maplibre.org/style.json',
  );
  // #endregion style

  // #region attach
  final session = map.attachOffscreen(
    const OffscreenDescriptor(
      logicalSize: LogicalSize(width: 800, height: 600),
    ),
    // The library renders on its own thread.
    const RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
    ),
  );
  await session.ready;
  // #endregion attach

  // #region image
  map.discarding.updateCamera(
    const CameraUpdate(
      mode: CameraUpdateMode.jump,
      camera: CameraOptions(
        center: LatLng(latitude: 48.8566, longitude: 2.3522),
        zoom: 4,
      ),
    ),
  );

  await map.requestStillImage();
  // #endregion image

  // #region read
  final image = await session.readTexture();
  copyPixels(image.data, image.info.stride);
  // #endregion read

  session.dispose();
  await map.close();
}

Future<void> imagesTile(
  RuntimeHandle runtime,
  int z,
  int x,
  int y,
) async {
  // #region tile
  final map = runtime.createMap(
    const MapOptions(
      initialExtent: LogicalExtent(
        width: 512,
        height: 512,
        scaleFactor: 1,
      ),
      mapMode: MapMode.tile,
    ),
  );
  await map.ready;

  map.discarding.updateCamera(
    CameraUpdate(
      mode: CameraUpdateMode.jump,
      camera: cameraForTile(
        CanonicalTileId(z: z, x: x, y: y),
        const LogicalSize(width: 512, height: 512),
      ),
    ),
  );
  // #endregion tile

  await map.close();
}
