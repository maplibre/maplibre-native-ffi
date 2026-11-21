/// Forced garbage collection through the VM service, for tests that check a
/// callback still runs after a collection.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:developer' as developer;
import 'dart:ffi';
import 'dart:io';
import 'dart:isolate';

import 'package:test/test.dart';

import 'fixture.dart';

/// Runs a full collection of this isolate's group.
///
/// The VM service serves on loopback only. A collection clears weak
/// references and runs native finalizers before its reply arrives.
Future<void> collectGarbage() => within(_collect(), 'garbage collection');

/// linux-gnu-riscv64 runs Dart tests under QEMU. Dart 3.14's VM service
/// isolate never publishes a loopback URI there, so forced GC cannot run.
final Object collectGarbageSkip = Abi.current() == Abi.linuxRiscv64
    ? 'Dart VM service has no loopback URI on linux-gnu-riscv64'
    : false;

/// How long one controlWebServer or WebSocket attempt may take.
///
/// QEMU (linux-gnu-riscv64) scales [waitDeadline] to 50s; a 2s cap aborted
/// enablement before the service isolate bound loopback.
Duration get _attemptTimeout {
  final scaled = waitDeadline ~/ 3;
  const floor = Duration(seconds: 2);
  return scaled < floor ? floor : scaled;
}

Future<WebSocket> _connectService() async {
  final deadline = DateTime.now().add(waitDeadline);
  Object? lastError;
  while (true) {
    // Dart 3.14 can enable the service with a null URI, return the URI
    // before loopback accepts the WebSocket, or hang in controlWebServer /
    // WebSocket.connect. All three showed up under QEMU (linux-gnu-riscv64).
    try {
      var info = await developer.Service.getInfo().timeout(_attemptTimeout);
      if (info.serverWebSocketUri == null) {
        info = await developer.Service.controlWebServer(
          enable: true,
          silenceOutput: true,
        ).timeout(_attemptTimeout);
      }
      final uri = info.serverWebSocketUri;
      if (uri != null) {
        final host = switch (uri.host) {
          'localhost' || '::1' => '127.0.0.1',
          final value => value,
        };
        return await WebSocket.connect(
          uri.replace(host: host).toString(),
        ).timeout(_attemptTimeout);
      }
    } on SocketException catch (error) {
      lastError = error;
    } on HttpException catch (error) {
      lastError = error;
    } on TimeoutException catch (error) {
      lastError = error;
    }
    if (DateTime.now().isAfter(deadline)) {
      if (lastError != null) {
        Error.throwWithStackTrace(lastError, StackTrace.current);
      }
      throw StateError('VM service did not publish a URI');
    }
    // Let the service isolate bind loopback; a tight await loop starves it.
    final idle = Completer<void>();
    Timer(Duration.zero, idle.complete);
    await idle.future;
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
