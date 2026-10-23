import 'dart:convert';
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

  // Caller-sized text that fits the runtime's first storage, and text that
  // does not, which the runtime fetches again with enough storage.
  check(utf8.decode(probeZerosJson(0)) == '[]', 'empty text');
  check(utf8.decode(probeZerosJson(3)) == '[0,0,0]', 'short text');
  check(
    utf8.decode(probeZerosJson(300)) == '[${List.filled(300, 0).join(',')}]',
    'long text',
  );

  // A caller-sized output array holds one element per input element, in
  // input order.
  const batch = [
    ProbeBatchPoint(x: 2, y: 5),
    ProbeBatchPoint(x: 7, y: 11),
    ProbeBatchPoint(x: -3, y: 17),
  ];
  final swapped = probeBatchPoints(batch);
  check(
    swapped.length == 3 &&
        swapped[0] == const ProbeBatchPoint(x: 5, y: 3) &&
        swapped[1] == const ProbeBatchPoint(x: 11, y: 8) &&
        swapped[2] == const ProbeBatchPoint(x: 17, y: -2),
    'caller-sized points: $swapped',
  );
  final scores = probeBatchScores(batch);
  check(
    scores.length == 3 && scores[0] == 7 && scores[1] == 18 && scores[2] == 14,
    'caller-sized scalars: $scores',
  );
  check(probeBatchPoints([]).isEmpty, 'empty caller-sized points');
  check(probeBatchScores([]).isEmpty, 'empty caller-sized scalars');

  final entry = keywordCombine(5, 2, 7, 11);
  check(
    entry == const KeywordEntry(type: 3, defer: 7, raw: 11),
    'keyword parameters: ${entry.type} ${entry.defer} ${entry.raw}',
  );

  // A record built from its constructor defaults equals the native default.
  probeSettingsCheck(ProbeSettings());

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
