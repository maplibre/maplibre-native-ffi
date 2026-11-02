// Snippets for the docs site's Create a map page. They are analyzed with
// the tests, which never run them.

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void reportFailure(MaplibreException error) {}

// Attaches a render target, as the render target guides show.
Future<RenderSessionHandle> attachTarget(MapHandle map) =>
    throw UnimplementedError();

Future<void> createAMap() async {
  // #region runtime
  final runtime = RuntimeHandle.create(const RuntimeOptions());
  // #endregion runtime

  // #region map
  final NewMap(:map, :ready) = runtime.createMap(
    const MapOptions(
      initialExtent: LogicalExtent(
        width: 800,
        height: 600,
        scaleFactor: 2,
      ),
      mapMode: MapMode.continuous,
    ),
  );
  await ready;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    'https://demotiles.maplibre.org/style.json',
  );
  // #endregion style

  const update = CameraUpdate(
    mode: CameraUpdateMode.jump,
    camera: CameraOptions(zoom: 3),
  );

  // #region wait
  await map.updateCamera(update);

  // Use the discarding variant to ignore the result.
  map.discarding.updateCamera(update);
  // #endregion wait

  // #region errors
  try {
    await map.updateCamera(update);
  } on MaplibreException catch (error) {
    // Its message describes the failure.
    reportFailure(error);
  }
  // #endregion errors

  final session = await attachTarget(map);

  // #region close
  session.dispose();
  map.dispose();
  await runtime.close();
  // #endregion close
}
