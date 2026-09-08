import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:test/test.dart';

void main() {
  for (final mode in ['awaited', 'unawaited']) {
    test('native callbacks permit process exit after $mode cleanup', () async {
      final process = await Process.start(Platform.resolvedExecutable, [
        'run',
        'test/fixtures/runtime_shutdown.dart',
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
        expect(await output, contains('CLOSED_ALL_HANDLES'));
      } on TimeoutException {
        process.kill();
        await process.exitCode;
        fail('Process remained alive.\n${await output}\n${await errors}');
      }
    });
  }
}
