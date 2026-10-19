/// The runtime and map fixture every binding test builds on.
///
/// Each handle a fixture opens is closed by a teardown the fixture registers,
/// so a failing test never leaks a runtime into the tests after it. Teardowns
/// run in reverse, so a map closes before the runtime that owns it.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

/// How long any single wait lasts before its test fails.
const waitDeadline = Duration(seconds: 10);

const emptyStyleJson = '{"version":8,"sources":{},"layers":[]}';

Uint8List jsonBytes(String value) => Uint8List.fromList(utf8.encode(value));

/// A flag that a callback or wake sets, and that a test awaits with a
/// deadline instead of polling.
final class Signal {
  var _completer = Completer<void>();
  var _set = false;

  void notify() {
    _set = true;
    if (!_completer.isCompleted) _completer.complete();
  }

  /// Returns once [notify] has run since the last wait, and resets.
  Future<void> wait(String what) async {
    if (!_set) {
      await _completer.future.timeout(
        waitDeadline,
        onTimeout: () => fail('timed out waiting for $what'),
      );
    }
    _set = false;
    _completer = Completer<void>();
  }
}

/// Awaits [future], failing the test when it does not finish in time.
Future<T> within<T>(Future<T> future, String what) => future.timeout(
  waitDeadline,
  onTimeout: () => fail('timed out waiting for $what'),
);

/// Answers every request with an error, so no test reaches the network.
///
/// A test that serves resources installs its own provider in place of this
/// one.
final denyingProvider = ResourceProvider(
  callback: (_, handle) {
    handle.complete(
      ResourceResponse(
        status: ResourceResponseStatus.error,
        errorMessage: 'the test fixture serves no resources',
      ),
    );
    handle.close();
  },
);

/// A runtime whose event wake drives [awaitEvent].
final class RuntimeFixture {
  RuntimeFixture._(this.runtime, this._events);

  final RuntimeHandle runtime;
  final Signal _events;

  /// Drains events until one matches, waiting on the event wake between
  /// drains. Events that do not match are dropped.
  Future<RuntimeEvent> awaitEvent(
    bool Function(RuntimeEvent event) matches,
    String what,
  ) async {
    while (true) {
      final batch = runtime.drainEvents();
      if (batch != null) {
        try {
          for (final event in batch.getValue().events) {
            if (matches(event)) return event;
          }
        } finally {
          batch.close();
        }
      }
      await _events.wait(what);
    }
  }

  Future<RuntimeEvent> awaitEventType(RuntimeEventType type) =>
      awaitEvent((event) => event.type == type, 'a $type event');

  /// Creates a map that a teardown closes.
  Future<MapHandle> openMap([MapOptions? options]) async {
    final map = await within(
      runtime.createMap(options ?? mapOptionsDefault()),
      'map creation',
    );
    addTearDown(map.close);
    return map;
  }

  /// Creates a map and loads [styleJson] into it.
  Future<MapHandle> openStyledMap([String styleJson = emptyStyleJson]) async {
    final map = await openMap();
    await expectCommitted(map.setStyleJson(jsonBytes(styleJson)));
    return map;
  }
}

/// Creates a runtime that a teardown closes, with the denying provider
/// installed unless [provider] names another.
Future<RuntimeFixture> openRuntime({
  RuntimeOptions? options,
  ResourceProvider? provider,
}) async {
  final defaults = options ?? runtimeOptionsDefault();
  final events = Signal();
  final runtime = runtimeCreate(
    RuntimeOptions(
      assetPath: defaults.assetPath,
      cachePath: defaults.cachePath,
      eventMask: defaults.eventMask,
      eventWake: Wake(callback: events.notify),
    ),
  );
  addTearDown(runtime.close);
  await runtime.setResourceProvider(provider ?? denyingProvider);
  return RuntimeFixture._(runtime, events);
}

Future<void> expectCommitted(Future<CommandCompletion> command) async {
  final completion = await within(command, 'a command');
  expect(
    completion.disposition,
    CommandDisposition.committed,
    reason: completion.diagnostic,
  );
}

/// Registers a provider that claims the requests its routes match. Native
/// loading takes every other request.
ResourceProvider routedProvider(
  List<ResourceRoute> routes,
  ResourceProviderCallback callback,
) => ResourceProvider(callback: callback, routes: routes);

/// A route that claims one style URL.
ResourceRoute styleRoute(String url) =>
    ResourceRoute(kinds: ResourceKindMask.style, url: url);

ResourceResponse get emptyStyleResponse => ResourceResponse(
  status: ResourceResponseStatus.ok,
  bytes: jsonBytes(emptyStyleJson),
);
