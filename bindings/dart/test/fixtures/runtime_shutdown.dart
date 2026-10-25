import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

import '../support/graphics.dart';

Future<void> closeRuntimes() async {
  for (var cycle = 0; cycle < 3; cycle++) {
    await Future.wait(
      List.generate(2, (_) async {
        final runtime = runtimeCreate(runtimeOptionsDefault());
        final NewMap(:map, :ready) = runtime.createMap(mapOptionsDefault());
        await ready;
        try {
          await runtime.close();
          throw StateError('runtime closed with a live map');
        } on MaplibreException catch (error) {
          if (error.status != Status.invalidState) rethrow;
          // A rejected close leaves the runtime available for later work.
        }
        await runtime.clearResourceTransform();
        await map.close();
        await runtime.close();
      }),
    );
  }
  print('CLOSED_ALL_HANDLES');
}

const _emptyStyle = {
  'version': 8,
  'sources': <String, Object?>{},
  'layers': <Object?>[],
};

/// Leaves a runtime and a map with a loaded style open, for the isolate's
/// shutdown to finalize.
Future<void> abandonHandles() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final NewMap(:map, :ready) = runtime.createMap(mapOptionsDefault());
  await ready;
  await map.setStyleJson(_emptyStyle);
  print('ABANDONED_HANDLES');
}

/// Leaves a session attached on its core worker after it rendered, with its
/// runtime and map, for the isolate's shutdown to finalize while the process
/// exits. The graphics context stays open, as a host's would until exit.
Future<void> abandonSession() async {
  final extent = LogicalExtent(width: 32, height: 32, scaleFactor: 1);
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final NewMap(:map, :ready) = runtime.createMap(
    MapOptions(initialExtent: extent),
  );
  await ready;
  await map.setStyleJson({
    'version': 8,
    'sources': <String, Object?>{},
    'layers': [
      {
        'id': 'background',
        'type': 'background',
        'paint': {'background-color': '#ff0000'},
      },
    ],
  });
  var frames = Completer<void>();
  final attachment = TestGraphics.create().attachOwnedTexture(
    map,
    LogicalExtent(width: 32, height: 32, scaleFactor: 1),
    RenderSessionAttachOptions(
      driver: RenderDriverKind.coreWorker,
      requestedTextureRingDepth: 1,
      frameWake: Wake(
        callback: () {
          if (!frames.isCompleted) frames.complete();
        },
      ),
    ),
  );
  final session = attachment.session;
  await attachment.ready;
  // A frame wake leaves the isolate free to finish, so a port keeps it alive
  // until a frame result arrives.
  final waiting = ReceivePort();
  session.requestFrame(FrameDemand(token: 1));
  var drained = 0;
  while (drained == 0) {
    await frames.future;
    frames = Completer<void>();
    // A wake that came before the result could be drained finds no results.
    drained = session.drainFrameResults().length;
  }
  waiting.close();
  print('ABANDONED_SESSION');
}

/// Leaves a runtime and a map open with every kind of Dart callback
/// registered, then ends the process through exit() while the runtime's
/// threads are still at work.
///
/// A callback registration leaves its isolate free to finish, so the fixture
/// keeps the isolate alive itself until the resource provider runs, with a
/// port that the provider closes.
Future<void> exitWithLiveCallbacks() async {
  logSetCallback(LogHandler(callback: (_, _, _, _) {}));
  final runtime = runtimeCreate(runtimeOptionsDefault());
  final waiting = ReceivePort();
  final served = Completer<void>();
  await runtime.setResourceProvider(
    ResourceProvider(
      routes: [
        ResourceRoute(kinds: ResourceKindFlag.style, url: 'shutdown://style'),
      ],
      callback: (_, request) {
        request.complete(
          ResourceResponse(
            status: ResourceResponseStatus.ok,
            bytes: utf8.encode(jsonEncode(_emptyStyle)),
          ),
        );
        waiting.close();
        served.complete();
      },
    ),
  );
  final NewMap(:map, :ready) = runtime.createMap(mapOptionsDefault());
  await ready;
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
    case 'session':
      await abandonSession();
    case 'exit':
      await exitWithLiveCallbacks();
  }
}
