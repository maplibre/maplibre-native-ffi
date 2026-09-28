import 'dart:convert';
import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:hooks/hooks.dart';
import 'package:test/test.dart';

import '../hook/build.dart' as hook;

void main() {
  test('bundled asset cleanup preserves shared native installs', () async {
    final temporary = await Directory.systemTemp.createTemp('native-hook-');
    addTearDown(() => temporary.delete(recursive: true));
    final packageRoot = temporary.uri.resolve('package/');
    final pointer = File.fromUri(
      packageRoot.resolve(hook.installPrefixPointer),
    );
    await pointer.parent.create(recursive: true);
    final builder = BuildInputBuilder()
      ..setupShared(
        packageRoot: packageRoot,
        packageName: 'maplibre_native_ffi',
        outputDirectoryShared: temporary.uri.resolve('output/'),
        outputFile: temporary.uri.resolve('output.json'),
      )
      ..setupBuildInput();
    builder.config.setupBuild(linkingEnabled: false);
    CodeAssetExtension(
      targetArchitecture: Architecture.arm64,
      targetOS: OS.android,
      linkModePreference: LinkModePreference.dynamic,
      android: AndroidCodeConfig(targetNdkApi: 24),
    ).setupBuildInput(builder);
    final input = builder.build();
    final config = File.fromUri(temporary.uri.resolve('input.json'));
    await config.writeAsString(jsonEncode(input.json));
    final sources = <File>[];
    for (final backend in ['opengl', 'vulkan']) {
      final prefix = temporary.uri.resolve('$backend/');
      final descriptor = File.fromUri(
        prefix.resolve('share/maplibre-native-c/artifact.json'),
      );
      await descriptor.parent.create(recursive: true);
      await descriptor.writeAsString(
        jsonEncode({
          'targetPlatform': 'android-arm64',
          'renderBackend': backend,
        }),
      );
      for (final name in ['libmaplibre-native-c.so', 'libdependency.so']) {
        final source = File.fromUri(prefix.resolve('lib/$name'));
        await source.parent.create(recursive: true);
        await source.writeAsString('$backend/$name');
        sources.add(source);
      }
      await pointer.writeAsString(prefix.toFilePath());
      await hook.main(['--config=${config.path}']);
      final output = BuildOutput(
        jsonDecode(await File.fromUri(input.outputFile).readAsString())
            as Map<String, Object?>,
      );
      expect(output.assets.code, hasLength(2));
      for (final asset in output.assets.code) {
        final file = File.fromUri(asset.file!);
        expect(
          await file.readAsString(),
          '$backend/${file.uri.pathSegments.last}',
        );
        // Build tools may remove any declared output when configurations change.
        await file.delete();
      }
      for (final source in sources) {
        expect(source.existsSync(), isTrue, reason: source.path);
        expect(
          await source.readAsString(),
          contains(source.uri.pathSegments.last),
        );
      }
    }
  });
}
