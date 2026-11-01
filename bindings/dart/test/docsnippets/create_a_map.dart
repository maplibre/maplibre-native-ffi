// Snippets for the docs site's Create a map page. They are analyzed with
// the tests, which never run them.
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void copyPixels(Uint8List data, int stride) {}

Future<void> createAMap() async {
  // #region runtime
  final runtime = RuntimeHandle.create(const RuntimeOptions());
  // #endregion runtime

  // #region map
  final extent = LogicalExtent(width: 512, height: 512, scaleFactor: 1);
  final NewMap(:map, ready: mapReady) = runtime.createMap(
    MapOptions(initialExtent: extent, mapMode: MapMode.static),
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
        GraphicsDeviceTextureDescriptor(extent: extent),
        // The library renders on its own thread.
        const RenderSessionAttachOptions(driver: RenderDriverKind.coreWorker),
      );
  await sessionReady;
  // #endregion attach

  // #region render
  await map.requestStillImage();
  // #endregion render

  // #region render
  final image = await session.readTexture();
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
