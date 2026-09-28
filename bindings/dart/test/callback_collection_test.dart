import 'dart:async';
import 'dart:convert';
import 'dart:developer' as developer;
import 'dart:io';
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

WeakReference<RuntimeHandle> _unreachableRuntime() {
  late final RuntimeHandle runtime;
  runtime = runtimeCreate(
    RuntimeOptions(
      eventWake: Wake(
        callback: () {
          runtime.identity;
        },
      ),
    ),
  );
  return WeakReference(runtime);
}

Future<WeakReference<RuntimeHandle>> _unreachableProvider() async {
  final runtime = runtimeCreate(runtimeOptionsDefault());
  await runtime.setQueuedResourceProvider(
    QueuedResourceProvider(
      routes: const [
        ResourceProviderRoute(kind: ResourceKind.style, url: 'capture://style'),
      ],
      callback: (_, request) {
        runtime.identity;
        request.close();
      },
    ),
  );
  return WeakReference(runtime);
}

void main() {
  test(
    'native callback ports do not keep self-capturing owners alive',
    () async {
      final previous = await developer.Service.getInfo();
      final service = await developer.Service.controlWebServer(enable: true);
      final uri = service.serverUri!;
      final socket = await WebSocket.connect(
        uri.replace(scheme: 'ws', path: '${uri.path}ws').toString(),
      );
      final replies = StreamIterator(
        socket.map(
          (message) => jsonDecode(message as String) as Map<String, dynamic>,
        ),
      );
      try {
        final references = [
          _unreachableRuntime(),
          await _unreachableProvider(),
        ];
        for (var attempt = 0; attempt < 10; attempt++) {
          socket.add(
            jsonEncode({
              'jsonrpc': '2.0',
              'id': '$attempt',
              'method': 'getAllocationProfile',
              'params': {
                'isolateId': developer.Service.getIsolateId(Isolate.current),
                'gc': true,
              },
            }),
          );
          do {
            expect(await replies.moveNext(), isTrue);
          } while (replies.current['id'] != '$attempt');
          expect(replies.current['error'], isNull);
          if (references.every((reference) => reference.target == null)) break;
          await Future<void>.delayed(Duration.zero);
        }
        expect(
          references.map((reference) => reference.target),
          everyElement(isNull),
          reason: 'the native port retained the callback and its owner',
        );
      } finally {
        await replies.cancel();
        await socket.close();
        if (previous.serverUri == null) {
          await developer.Service.controlWebServer(enable: false);
        }
      }
    },
  );
}
