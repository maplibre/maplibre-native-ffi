import 'dart:io';

import 'values.dart';
import 'runtime.dart';

void check(bool condition, String message) {
  if (!condition) {
    stderr.writeln(message);
    exit(1);
  }
}

void main() {
  const point = ProbePoint(type: 9.5, gain: 3.25);
  final input = ProbeOptions(
    title: '',
    point: point,
    left: [point],
    right: [point, point],
  );
  final output = probeRoundtrip(input);
  check(output == input, 'round trip: ${output.title} ${output.left}');

  final empty = probeRoundtrip(ProbeOptions(left: [], right: []));
  check(empty.left != null && empty.left!.isEmpty, 'present empty list');

  final absent = probeRoundtrip(ProbeOptions(right: []));
  check(
    absent.title == null &&
        absent.point == null &&
        absent.left == null &&
        absent.right.isEmpty,
    'absent fields',
  );

  final entry = keywordCombine(5, 2, 7, 11);
  check(
    entry == const KeywordEntry(type: 3, defer: 7, raw: 11),
    'keyword parameters: ${entry.type} ${entry.defer} ${entry.raw}',
  );

  // A record built from its constructor defaults equals the native default.
  probeSettingsCheck(const ProbeSettings());

  // A copy of a native default keeps its values and leaves a registration
  // unset.
  final hooks = probeHooksDefault();
  check(
    hooks.limit == 4 && hooks.signal == const ProbeSignal(),
    'hooks default: ${hooks.limit} ${hooks.signal}',
  );

  // A status that the C API declares as absence returns null, and any other
  // failure still reports its diagnostic.
  check(probeReadLevel() == null, 'absent level');
  final level = probeReadLevel();
  check(level == 0.5, 'published level: $level');
  try {
    probeReadLevel();
    check(false, 'an exhausted level was not reported');
  } on NativeFailure catch (failure) {
    check(
      failure.status == -1 && failure.message == 'exhausted',
      'exhausted level: ${failure.status} ${failure.message}',
    );
  }

  try {
    probeRoundtrip(ProbeOptions(right: List.filled(9, point)));
    check(false, 'native failure was not reported');
  } on NativeFailure catch (failure) {
    check(
      failure.status == -1 &&
          failure.message == 'right holds more than 8 points',
      'failure: ${failure.status} ${failure.message}',
    );
  }
}
