// How a process ends with handles and callbacks still live.
import 'dart:io';

import 'package:test/test.dart';

import 'support/fixture_process.dart';

void main() {
  late File executable;
  setUpAll(() async {
    executable = await compileFixture('test/fixtures/runtime_shutdown.dart');
  });

  test(
    'pending completions keep the process alive until unawaited cleanup ends',
    () => expectCleanExit(executable, 'unawaited', 'CLOSED_ALL_HANDLES'),
  );

  test('shutdown with live handles and callbacks exits cleanly', () async {
    // The isolate runs out of work with a runtime and a map open, and its
    // shutdown finalizes them.
    await expectCleanExit(executable, 'abandoned', 'ABANDONED_HANDLES');
    // Shutdown also finalizes a session still attached to its core worker,
    // without graphics calls that would race the drivers' exit teardown.
    await expectCleanExit(executable, 'session', 'ABANDONED_SESSION');
    // exit() ends a process with live callback registrations and the
    // runtime's threads still at work.
    await expectCleanExit(executable, 'exit', 'LIVE_CALLBACKS');
  });
}
