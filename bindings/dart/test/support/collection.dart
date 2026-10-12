/// Forced garbage collection through the VM service, for tests of what an
/// owner keeps reachable and what a native finalizer disposes.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:developer' as developer;
import 'dart:io';
import 'dart:isolate';

import 'package:test/test.dart';

import 'fixture.dart';

/// How many full collections [collectGarbageUntil] runs before it fails.
const _collections = 10;

/// Runs full collections of this isolate's group until [collected] holds.
///
/// The VM service serves on loopback only. A collection clears weak
/// references and runs native finalizers before its reply arrives.
Future<void> collectGarbageUntil(bool Function() collected, String what) =>
    within(_collectUntil(collected, what), 'garbage collection of $what');

Future<void> _collectUntil(bool Function() collected, String what) async {
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
    for (var attempt = 0; attempt < _collections; attempt++) {
      if (collected()) return;
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
    }
    if (!collected()) fail('$_collections collections left $what reachable');
  } finally {
    await replies.cancel();
    await socket.close();
    if (previous.serverUri == null) {
      await developer.Service.controlWebServer(enable: false);
    }
  }
}
