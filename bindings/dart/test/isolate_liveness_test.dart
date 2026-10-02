// What keeps an isolate alive: a pending completion does, and a callback
// registration does not.
import 'dart:io';

import 'package:test/test.dart';

import 'support/fixture_process.dart';

void main() {
  late File executable;
  setUpAll(() async {
    executable = await compileFixture('test/fixtures/isolate_liveness.dart');
  });

  test(
    'an awaited completion resolves while only registrations are live',
    () => expectCleanExit(executable, 'completions', 'COMPLETIONS_RESOLVED'),
  );

  test(
    'an isolate whose only live state is callback registrations exits',
    () => expectCleanExit(executable, 'registrations', 'REGISTRATIONS_LIVE'),
  );
}
