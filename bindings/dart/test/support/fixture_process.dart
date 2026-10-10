/// Fixture programs that a test runs as a separate Dart process, for behavior
/// that only a whole isolate's lifetime shows, such as how it ends.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:frontend_server_client/frontend_server_client.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

/// How long a fixture process may run before the test fails it.
const _processDeadline = Duration(seconds: 20);

/// Compiles the fixture at [source] into a kernel file that a teardown
/// deletes.
///
/// The kernel reuses the suite's native assets, because another `dart run`
/// would overwrite DLLs that this process has already loaded on Windows.
Future<File> compileFixture(String source) async {
  // Resolving the native library makes the suite's native assets exist.
  supportedRenderBackendMask();
  final directory = await Directory.systemTemp.createTemp('dart-fixture-');
  addTearDown(() => directory.delete(recursive: true));
  final executable = File.fromUri(directory.uri.resolve('fixture.dill'));
  final compiler = await FrontendServerClient.start(
    source,
    executable.path,
    'lib/_internal/vm_platform_strong.dill',
    nativeAssets: '.dart_tool/native_assets.yaml',
  );
  try {
    final result = await compiler.compile();
    expect(result.errorCount, 0, reason: result.compilerOutputLines.join('\n'));
  } finally {
    await compiler.shutdown();
  }
  return executable;
}

/// Runs [executable] in [mode] and expects it to exit cleanly on its own,
/// after it printed [marker] and reported no failed finalization.
Future<void> expectCleanExit(
  File executable,
  String mode,
  String marker,
) async {
  final process = await Process.start(Platform.resolvedExecutable, [
    executable.path,
    mode,
  ]);
  addTearDown(() => process.kill());
  final output = process.stdout.transform(utf8.decoder).join();
  final errors = process.stderr.transform(utf8.decoder).join();
  try {
    final exitCode = await process.exitCode.timeout(_processDeadline);
    expect(exitCode, 0, reason: await errors);
    expect(await output, contains(marker));
    expect(await errors, isNot(contains('finalization failed')));
  } on TimeoutException {
    process.kill();
    await process.exitCode;
    fail('Process remained alive.\n${await output}\n${await errors}');
  }
}
