// Snippets for the docs site's Events and logs page. They are analyzed with
// the tests, which never run them.

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void onStyleLoaded() {}

void onLoadingFailed(String message) {}

void writeLog(LogSeverity severity, String message) {}

Future<void> eventsAndLogs(RuntimeHandle runtime) async {
  MapHandle? map;

  // #region handler
  // #region queue
  // The handler runs in this isolate. The binding queues each call.
  runtime.discarding.setEventHandler((events) {
    for (final event in events) {
      if (event.source != map?.id) continue;

      switch (event.type) {
        case RuntimeEventType.mapStyleLoaded:
          onStyleLoaded();
        case RuntimeEventType.mapLoadingFailed:
          onLoadingFailed(event.message);
      }
    }
  });
  // #endregion queue
  // #endregion handler

  // #region mask
  final created = runtime.createMap(
    MapOptions(
      initialExtent: const LogicalExtent(
        width: 512,
        height: 512,
        scaleFactor: 1,
      ),
      eventMask:
          RuntimeEventFlag.mapStyleLoaded | RuntimeEventFlag.mapLoadingFailed,
    ),
  );
  // #endregion mask
  map = created.map;
  await created.ready;

  // #region logs
  // The callback runs in this isolate. The binding queues each call.
  setLogCallback((severity, _, _, message) {
    writeLog(severity, message);
  });
  // #endregion logs
}
