/// Forced garbage collection through the VM service, for tests that check a
/// callback still runs after a collection.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:developer' as developer;
import 'dart:io';
import 'dart:isolate';

import 'package:test/test.dart';

import 'fixture.dart';

/// Runs a full collection of this isolate's group.
///
/// The VM service serves on loopback only. A collection clears weak
/// references and runs native finalizers before its reply arrives.
Future<void> collectGarbage() => within(_collect(), 'garbage collection');

Future<WebSocket> _connectService(String uri) async {
  final deadline = DateTime.now().add(waitDeadline);
  while (true) {
    try {
      return await WebSocket.connect(uri);
    } on SocketException {
      if (DateTime.now().isAfter(deadline)) {
        rethrow;
      }
    }
  }
}

Future<void> _collect() async {
  final previous = await developer.Service.getInfo();
  final service = await developer.Service.controlWebServer(enable: true);
  final uri = service.serverUri!;
  // Dart 3.14 can return this URI before loopback accepts the WebSocket.
  final socket = await _connectService(
    uri.replace(scheme: 'ws', path: '${uri.path}ws').toString(),
  );
  final replies = StreamIterator(
    socket.map(
      (message) => jsonDecode(message as String) as Map<String, dynamic>,
    ),
  );
  try {
    socket.add(
      jsonEncode({
        'jsonrpc': '2.0',
        'id': 'gc',
        'method': 'getAllocationProfile',
        'params': {
          'isolateId': developer.Service.getIsolateId(Isolate.current),
          'gc': true,
        },
      }),
    );
    do {
      expect(await replies.moveNext(), isTrue);
    } while (replies.current['id'] != 'gc');
    expect(replies.current['error'], isNull);
  } finally {
    await replies.cancel();
    await socket.close();
    if (previous.serverUri == null) {
      await developer.Service.controlWebServer(enable: false);
    }
  }
}
