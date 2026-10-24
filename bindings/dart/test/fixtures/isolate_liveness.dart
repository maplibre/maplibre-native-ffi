import 'dart:convert';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

const _styleJson = '{"version":8,"sources":{},"layers":[]}';

Uint8List _json(String value) => Uint8List.fromList(utf8.encode(value));

/// Registers each kind of long-lived callback: the log callback, an event
/// wake, a resource transform, a resource provider, and a custom source.
///
/// Each registration that native confirms returns a future, and that pending
/// completion is what keeps the isolate alive until it resolves.
Future<(RuntimeHandle, MapHandle)> _register() async {
  logSetCallback(LogHandler(callback: (_, _, _, _) {}));
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setResourceTransform(
    ResourceTransform(callback: (_, _, request) => request.close()),
  );
  await runtime.setResourceProvider(
    ResourceProvider(
      routes: [
        ResourceRoute(kinds: ResourceKindMask.style, url: 'liveness://style'),
      ],
      callback: (_, request) => request.close(),
    ),
  );
  final NewMap(:map, :ready) = runtime.createMap(mapOptionsDefault());
  await ready;
  await map.setStyleJson(_json(_styleJson));
  await map.addCustomGeometrySource(
    'source',
    CustomGeometrySourceOptions(fetchTile: (_) {}),
  );
  return (runtime, map);
}

/// Awaits completions while only callback registrations are otherwise live,
/// then closes everything, so that the exit rests on nothing else.
Future<void> _awaitCompletions() async {
  final (runtime, map) = await _register();
  await runtime.clearResourceTransform();
  print('COMPLETIONS_RESOLVED');
  await map.close();
  await runtime.close();
  logClearCallback();
}

/// Leaves every registration live and returns, so the isolate runs out of
/// work with nothing pending but those registrations.
Future<void> _leaveRegistrations() async {
  await _register();
  print('REGISTRATIONS_LIVE');
}

Future<void> main(List<String> arguments) async {
  switch (arguments.single) {
    case 'completions':
      await _awaitCompletions();
    case 'registrations':
      await _leaveRegistrations();
  }
}
