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

Future<WebSocket> _connectService() async {
  final deadline = DateTime.now().add(waitDeadline);
  while (true) {
    // Dart 3.14 can enable the service with a null URI, or return the URI
    // before loopback accepts the WebSocket. Both showed up under QEMU.
    final service = await developer.Service.controlWebServer(enable: true);
    final uri = service.serverUri;
    if (uri != null) {
      try {
        return await WebSocket.connect(
          uri.replace(scheme: 'ws', path: '${uri.path}ws').toString(),
        );
      } on SocketException {
        if (DateTime.now().isAfter(deadline)) {
          rethrow;
        }
      } on HttpException {
        if (DateTime.now().isAfter(deadline)) {
          rethrow;
        }
      }
    } else if (DateTime.now().isAfter(deadline)) {
      throw StateError('VM service did not publish a URI');
    }
  }
}

Future<void> _collect() async {
  final socket = await _connectService();
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
    // Leave the VM service up. Dart 3.14 runs tests in concurrent isolates
    // that share loopback, and shutting the server races the next collection.
  }
}
