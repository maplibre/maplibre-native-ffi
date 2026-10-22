import 'dart:convert';
import 'dart:io';

import 'package:archive/archive.dart';
import 'package:code_assets/code_assets.dart';
import 'package:crypto/crypto.dart';
import 'package:hooks/hooks.dart';
import 'package:test/test.dart';

import '../hook/build.dart' as hook;

CodeAssetExtension _android(Architecture architecture) => CodeAssetExtension(
  targetArchitecture: architecture,
  targetOS: OS.android,
  linkModePreference: LinkModePreference.dynamic,
  android: AndroidCodeConfig(targetNdkApi: 24),
);

final _ios = CodeAssetExtension(
  targetArchitecture: Architecture.arm64,
  targetOS: OS.iOS,
  linkModePreference: LinkModePreference.dynamic,
  iOS: IOSCodeConfig(targetSdk: IOSSdk.iPhoneOS, targetVersion: 13),
);

void main() {
  late Directory temporary;
  setUp(() async {
    temporary = await Directory.systemTemp.createTemp('native-hook-');
    addTearDown(() => temporary.delete(recursive: true));
  });

  /// Runs the hook for [target] with [defines] as the root pubspec's
  /// user-defines, which resolve paths against the temporary directory.
  Future<BuildOutput> runHook(
    CodeAssetExtension target,
    Map<String, Object?> defines,
  ) async {
    final builder = BuildInputBuilder()
      ..setupShared(
        packageRoot: temporary.uri.resolve('package/'),
        packageName: 'maplibre_native_ffi',
        outputDirectoryShared: temporary.uri.resolve('shared/'),
        outputFile: temporary.uri.resolve('output.json'),
        userDefines: PackageUserDefines(
          workspacePubspec: PackageUserDefinesSource(
            defines: defines,
            basePath: temporary.uri,
          ),
        ),
      )
      ..setupBuildInput();
    builder.config.setupBuild(linkingEnabled: false);
    target.setupBuildInput(builder);
    final input = builder.build();
    final config = File.fromUri(temporary.uri.resolve('input.json'));
    await config.writeAsString(jsonEncode(input.json));
    await hook.main(['--config=${config.path}']);
    return BuildOutput(
      jsonDecode(await File.fromUri(input.outputFile).readAsString())
          as Map<String, Object?>,
    );
  }

  test('bundled asset cleanup preserves shared native installs', () async {
    final sources = <File>[];
    for (final backend in ['opengl', 'vulkan']) {
      sources.addAll(
        _writePrefix(
          temporary.uri.resolve('$backend/'),
          'android-arm64',
          backend,
          libraries: ['libmaplibre-native-c.so', 'libdependency.so'],
        ),
      );
      final output = await runHook(_android(Architecture.arm64), {
        'local_build': backend,
      });
      expect(output.assets.code, hasLength(2));
      for (final asset in output.assets.code) {
        final file = File.fromUri(asset.file!);
        expect(await file.readAsString(), 'android-arm64-$backend');
        // Build tools may remove any declared output when configurations change.
        await file.delete();
      }
      for (final source in sources) {
        expect(source.existsSync(), isTrue, reason: source.path);
      }
    }
  });

  test('a build root serves each target the preset for its backend', () async {
    for (final (platform, backend) in [
      ('android-arm64', 'egl'),
      ('android-arm64', 'vulkan'),
      ('android-x64', 'vulkan'),
      ('ios-arm64', 'metal'),
    ]) {
      _writePrefix(
        temporary.uri.resolve('build/$platform-$backend/install/'),
        platform,
        backend == 'egl' ? 'opengl' : backend,
        libraries: [
          platform.startsWith('ios')
              ? 'libmaplibre-native-c.dylib'
              : 'libmaplibre-native-c.so',
        ],
      );
    }

    final defines = {
      'local_build': 'build',
      'backend': {'android': 'vulkan'},
    };
    for (final (target, preset) in [
      (_android(Architecture.arm64), 'android-arm64-vulkan'),
      (_android(Architecture.x64), 'android-x64-vulkan'),
      (_ios, 'ios-arm64-metal'),
    ]) {
      final output = await runHook(target, defines);
      expect(
        await File.fromUri(output.assets.code.single.file!).readAsString(),
        contains(preset),
      );
    }
  });

  group('a published package', () {
    late HttpServer server;
    final served = <String, List<int>>{};
    setUp(() async {
      served.clear();
      server = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
      server.listen((request) async {
        final body = served[request.uri.path];
        request.response.statusCode = body == null
            ? HttpStatus.notFound
            : HttpStatus.ok;
        if (body != null) {
          request.response.add(body);
        }
        await request.response.close();
      });
      addTearDown(() => server.close(force: true));
    });

    /// Serves [preset]'s archive in the release [tag] and pins it in the
    /// package, under [digest] when given and its own digest otherwise.
    void release(String tag, String preset, {String? digest}) {
      final root = 'maplibre-native-c-$preset';
      final archive = Archive()
        ..addFile(
          ArchiveFile.string('$root/lib/libmaplibre-native-c.so', preset),
        );
      final bytes = GZipEncoder().encode(TarEncoder().encode(archive));
      served['/$tag/$root.tar.gz'] = bytes;
      File.fromUri(temporary.uri.resolve('package/hook/native_release.json'))
        ..parent.createSync(recursive: true)
        ..writeAsStringSync(
          jsonEncode({
            'tag': tag,
            'sha256': {
              '$root.tar.gz': digest ?? sha256.convert(bytes).toString(),
            },
          }),
        );
    }

    Map<String, Object?> mirror() => {
      'release_url': 'http://${server.address.host}:${server.port}/',
    };

    test('downloads its pinned archive once', () async {
      release('v0.202610.2', 'android-arm64-egl');
      final output = await runHook(_android(Architecture.arm64), mirror());
      expect(
        await File.fromUri(output.assets.code.single.file!).readAsString(),
        'android-arm64-egl',
      );

      // A later build unpacks nothing and reaches no server.
      served.clear();
      final again = await runHook(_android(Architecture.arm64), mirror());
      expect(
        await File.fromUri(again.assets.code.single.file!).readAsString(),
        'android-arm64-egl',
      );
    });

    test('rejects an archive that differs from its pinned digest', () async {
      release('v0.202610.2', 'android-arm64-egl', digest: '0' * 64);
      await expectLater(
        runHook(_android(Architecture.arm64), mirror()),
        throwsStateError,
      );
    });
  });
}

/// Writes an install prefix's descriptor and [libraries], each library holding
/// the platform and backend that it stands in for, and returns the files.
List<File> _writePrefix(
  Uri prefix,
  String platform,
  String backend, {
  required List<String> libraries,
}) {
  File write(String name, String content) => File.fromUri(prefix.resolve(name))
    ..parent.createSync(recursive: true)
    ..writeAsStringSync(content);
  return [
    write(
      'share/maplibre-native-c/artifact.json',
      jsonEncode({'targetPlatform': platform, 'renderBackend': backend}),
    ),
    for (final library in libraries)
      write('lib/$library', '$platform-$backend'),
  ];
}
