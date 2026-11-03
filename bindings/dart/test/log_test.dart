// The process-global log callback. This is the only suite that installs one,
// so suites that dart test runs alongside it never see its registrations.
@Tags(['global_state'])
library;

import 'dart:async';
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

import 'support/fixture.dart';

/// Whether a log message is one of the two rules around a debug-log dump.
bool _isDumpRule(String message) => message.startsWith('-----');

/// Installs a log callback that collects dump rules, and returns them.
List<(LogSeverity, LogEvent)> _collectDumpRules() {
  final rules = <(LogSeverity, LogEvent)>[];
  setLogCallback((severity, event, _, message) {
    if (_isDumpRule(message)) rules.add((severity, event));
  });
  return rules;
}

void _clearLogCallback() => clearLogCallback();

void main() {
  late MapHandle map;

  setUp(() async {
    // MapLibre delivers info records from a worker by default, and a record
    // from one dump could then reach the callback installed for the next.
    // Synchronous records reach the delivery queue before the dump's own
    // completion does, so awaiting a dump fences its records.
    setLogAsyncSeverityMask(LogSeverityFlag.none);
    addTearDown(() {
      clearLogCallback();
      setLogAsyncSeverityMask(LogSeverityFlag.defaultValue);
    });
    final fixture = await openRuntime();
    map = await fixture.openMap();
  });

  Future<void> dump() => expectCommitted(map.dumpDebugLogs());

  test('a replaced or cleared log callback receives no more records', () async {
    final first = _collectDumpRules();
    await dump();
    final rules = first.length;
    expect(rules, greaterThan(0));
    expect(first, everyElement((LogSeverity.info, LogEvent.general)));

    final replacement = _collectDumpRules();
    await dump();
    expect(replacement, hasLength(rules));
    expect(first, hasLength(rules));

    clearLogCallback();
    await dump();
    expect(replacement, hasLength(rules));
  });

  test(
    'another isolate can clear the registration this one installed',
    () async {
      final cleared = _collectDumpRules();
      // The registration is process-global, so another isolate clears it.
      await Isolate.run(_clearLogCallback);

      final later = _collectDumpRules();
      await dump();
      expect(later, isNotEmpty);
      expect(cleared, isEmpty);
    },
  );
}
