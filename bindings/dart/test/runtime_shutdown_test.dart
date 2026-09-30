import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:frontend_server_client/frontend_server_client.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  late File executable;
  setUpAll(() async {
    supportedRenderBackendMask();
    final directory = await Directory.systemTemp.createTemp('native-shutdown-');
    addTearDown(() => directory.delete(recursive: true));
    executable = File.fromUri(directory.uri.resolve('runtime_shutdown.dill'));
    // Reuse the suite's assets: another `dart run` would overwrite DLLs that
    // this process has already loaded on Windows.
    final compiler = await FrontendServerClient.start(
      'test/fixtures/runtime_shutdown.dart',
      executable.path,
      'lib/_internal/vm_platform_strong.dill',
      nativeAssets: '.dart_tool/native_assets.yaml',
    );
    try {
      final result = await compiler.compile();
      expect(
        result.errorCount,
        0,
        reason: result.compilerOutputLines.join('\n'),
      );
    } finally {
      await compiler.shutdown();
    }
  });

  /// Runs the fixture in [mode] and expects a clean exit that printed
  /// [marker] and reported no failed finalization.
  Future<void> expectCleanExit(String mode, String marker) async {
    final process = await Process.start(Platform.resolvedExecutable, [
      executable.path,
      mode,
    ]);
    addTearDown(() => process.kill());
    final output = process.stdout.transform(utf8.decoder).join();
    final errors = process.stderr.transform(utf8.decoder).join();
    try {
      final exitCode = await process.exitCode.timeout(
        const Duration(seconds: 20),
      );
      expect(exitCode, 0, reason: await errors);
      expect(await output, contains(marker));
      expect(await errors, isNot(contains('finalization failed')));
    } on TimeoutException {
      process.kill();
      await process.exitCode;
      fail('Process remained alive.\n${await output}\n${await errors}');
    }
  }

  test(
    'pending completions keep the process alive until unawaited cleanup ends',
    () => expectCleanExit('unawaited', 'CLOSED_ALL_HANDLES'),
  );

  test('shutdown with live handles and callbacks exits cleanly', () async {
    // The isolate runs out of work with a runtime and a map open, and its
    // shutdown finalizes them.
    await expectCleanExit('abandoned', 'ABANDONED_HANDLES');
    // exit() ends a process whose live callback registrations would otherwise
    // keep it running, with the runtime's threads still at work.
    await expectCleanExit('exit', 'LIVE_CALLBACKS');
  });
}
