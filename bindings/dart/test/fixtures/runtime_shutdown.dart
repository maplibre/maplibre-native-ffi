import 'dart:async';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

Future<void> closeRuntimes() async {
  for (var cycle = 0; cycle < 3; cycle++) {
    await Future.wait(
      List.generate(2, (_) async {
        final runtime = RuntimeHandle.create();
        final map = await runtime.createMap();
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

Future<void> main(List<String> arguments) async {
  if (arguments.single == 'unawaited') {
    unawaited(closeRuntimes());
  } else {
    await closeRuntimes();
  }
}
