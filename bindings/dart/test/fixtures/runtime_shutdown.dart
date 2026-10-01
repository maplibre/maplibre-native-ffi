import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
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

Uint8List _json(String value) => Uint8List.fromList(utf8.encode(value));

/// Leaves a runtime and a map with a loaded style open, for the isolate's
/// shutdown to finalize.
Future<void> abandonHandles() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final map = await runtime.mapCreate(mapOptionsDefault());
  await map.setStyleJson(_json('{"version":8,"sources":{},"layers":[]}'));
  print('ABANDONED_HANDLES');
}

/// Leaves a runtime and a map open with every kind of Dart callback
/// registered, then ends the process through exit() while the runtime's
/// threads are still at work.
///
/// A callback registration leaves its isolate free to finish, so the fixture
/// keeps the isolate alive itself until the resource provider runs, with a
/// port that the provider closes.
Future<void> exitWithLiveCallbacks() async {
  logSetCallback((_, _, _, _) {});
  final runtime = runtimeCreate(
    RuntimeOptions(eventWake: Wake(callback: () {})),
  );
  final waiting = ReceivePort();
  final served = Completer<void>();
  await runtime.setResourceProvider(
    ResourceProvider.routedResourceProvider(
      AdapterRoutedResourceProvider(
        routes: [
          AdapterResourceRoute(
            kind: ResourceKind.style.rawValue,
            url: 'shutdown://style',
          ),
        ],
        callback: (_, request) {
          request.complete(
            ResourceResponse(
              status: ResourceResponseStatus.ok,
              bytes: _json('{"version":8,"sources":{},"layers":[]}'),
            ),
          );
          waiting.close();
          served.complete();
        },
      ),
    ),
  );
  final map = await runtime.mapCreate(mapOptionsDefault());
  await map.setStyleUrl('shutdown://style');
  await served.future;
  print('LIVE_CALLBACKS');
  await stdout.flush();
  exit(0);
}

Future<void> main(List<String> arguments) async {
  switch (arguments.single) {
    case 'unawaited':
      unawaited(closeRuntimes());
    case 'abandoned':
      await abandonHandles();
    case 'exit':
      await exitWithLiveCallbacks();
  }
}
