// The process-global log callback. This is the only suite that installs one,
// so suites that dart test runs alongside it never see its registrations.
@Tags(['global_state'])
library;

import 'dart:async';
import 'dart:isolate';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:maplibre_native_ffi/src/runtime/runtime.dart'
    show globalCallbackPortProbeForTesting;
import 'package:test/test.dart';

import 'support/fixture.dart';

/// Whether a log message is one of the two rules around a debug-log dump.
bool _isDumpRule(String message) => message.startsWith('-----');

/// Installs a log callback that collects dump rules, and returns them.
List<(LogSeverity, LogEvent)> _collectDumpRules() {
  final rules = <(LogSeverity, LogEvent)>[];
  logSetCallback((severity, event, _, message) {
    if (_isDumpRule(message)) rules.add((severity, event));
  });
  return rules;
}

void _clearLogCallback() => logClearCallback();

void main() {
  late MapHandle map;

  setUp(() async {
    // MapLibre delivers info records from a worker by default, and a record
    // from one dump could then reach the callback installed for the next.
    // Synchronous records reach the callback's port before the dump's own
    // completion does, so awaiting a dump fences its records.
    logSetAsyncSeverityMask(const LogSeverityMask.fromRawValue(0));
    addTearDown(() async {
      final installed = globalCallbackPortProbeForTesting();
      logClearCallback();
      logSetAsyncSeverityMask(LogSeverityMask.defaultValue);
      // The next test finds no registration of this one still pending.
      if (installed != null) {
        await within(installed.released, 'the log registration release');
      }
    });
    final fixture = await openRuntime();
    map = await fixture.openMap();
  });

  Future<void> dump() => expectCommitted(map.dumpDebugLogs());

  test(
    'replacing the log callback releases the previous registration',
    () async {
      final first = _collectDumpRules();
      final firstPort = globalCallbackPortProbeForTesting()!;
      await dump();
      final rules = first.length;
      expect(rules, greaterThan(0));
      expect(first, everyElement((LogSeverity.info, LogEvent.general)));

      final replacement = _collectDumpRules();
      await within(firstPort.released, 'the replaced registration release');
      await dump();
      expect(replacement, hasLength(rules));
      expect(first, hasLength(rules));

      final replacementPort = globalCallbackPortProbeForTesting()!;
      logClearCallback();
      await within(
        replacementPort.released,
        'the cleared registration release',
      );
      await dump();
      expect(replacement, hasLength(rules));
    },
  );

  test(
    'another isolate can clear the registration this one installed',
    () async {
      final cleared = _collectDumpRules();
      final clearedPort = globalCallbackPortProbeForTesting()!;
      // The registration is process-global, so another isolate clears it, and
      // native releases it to this isolate's port.
      await Isolate.run(_clearLogCallback);
      await within(clearedPort.released, 'the cleared registration release');

      final later = _collectDumpRules();
      await dump();
      expect(later, isNotEmpty);
      expect(cleared, isEmpty);
    },
  );

  test('a log callback error reaches the zone that installed it', () async {
    final zoneErrors = <Object>[];
    runZonedGuarded(
      () => logSetCallback((_, _, _, _) => throw StateError('log failed')),
      (error, _) => zoneErrors.add(error),
    );
    await dump();
    expect(zoneErrors, isNotEmpty);
    expect(zoneErrors, everyElement(isA<StateError>()));
  });
}
