// Snippets for the docs site's Create a map page. They are analyzed with
// the tests, which never run them.

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

// Attaches a render target, as the Rendering guides show.
Future<RenderSessionHandle> attachTarget(MapHandle map) =>
    throw UnimplementedError();

Future<void> createAMap() async {
  // #region runtime
  final runtime = RuntimeHandle.create(const RuntimeOptions());
  // #endregion runtime

  // #region map
  final NewMap(:map, :ready) = runtime.createMap(
    const MapOptions(
      initialExtent: LogicalExtent(width: 800, height: 600, scaleFactor: 2),
      mapMode: MapMode.continuous,
    ),
  );
  await ready;
  // #endregion map

  final session = await attachTarget(map);

  // #region close
  session.dispose();
  await map.close();
  await runtime.close();
  // #endregion close
}
