import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

Future<void> closeRuntimes() async {
  for (var cycle = 0; cycle < 3; cycle++) {
    await Future.wait(
      List.generate(2, (_) async {
        final runtime = runtimeCreate(runtimeOptionsDefault());
        final map = await runtime.mapCreate(mapOptionsDefault());
        try {
          await runtime.close();
          throw StateError('runtime closed with a live map');
        } on InvalidStateException {
          // A rejected close leaves the runtime available for later work.
        }
        await runtime.barrier();
        await map.close();
        await runtime.close();
      }),
    );
  }
  print('CLOSED_ALL_HANDLES');
}

/// Leaves a runtime and a map with a loaded style open, for the isolate's
/// shutdown to finalize.
Future<void> abandonHandles() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final map = await runtime.mapCreate(mapOptionsDefault());
  await map.setStyleJson(
    Uint8List.fromList(utf8.encode('{"version":8,"sources":{},"layers":[]}')),
  );
  print('ABANDONED_HANDLES');
}

Future<void> main(List<String> arguments) async {
  switch (arguments.single) {
    case 'unawaited':
      unawaited(closeRuntimes());
    case 'awaited':
      await closeRuntimes();
    case 'abandoned':
      await abandonHandles();
  }
}
