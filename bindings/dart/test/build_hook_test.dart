import 'dart:convert';
import 'dart:io';

import 'package:archive/archive.dart';
import 'package:code_assets/code_assets.dart';
import 'package:crypto/crypto.dart';
import 'package:hooks/hooks.dart';
import 'package:test/test.dart';

import '../hook/build.dart' as hook;

final _android = CodeAssetExtension(
  targetArchitecture: Architecture.arm64,
  targetOS: OS.android,
  linkModePreference: LinkModePreference.dynamic,
  android: AndroidCodeConfig(targetNdkApi: 24),
);

final _windows = CodeAssetExtension(
  targetArchitecture: Architecture.x64,
  targetOS: OS.windows,
  linkModePreference: LinkModePreference.dynamic,
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
          backend,
          libraries: ['libmaplibre-native-c.so', 'libdependency.so'],
        ),
      );
      final output = await runHook(_android, {'local_build': backend});
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

  /// Reads the first library that [output] declares, which names the platform
  /// and backend that it stands in for.
  Future<String> bundled(BuildOutput output) =>
      File.fromUri(output.assets.code.first.file!).readAsString();

  /// Writes the Android arm64 install for [backend] under the build root, in
  /// the CMake layout.
  void build(String backend) {
    _writePrefix(
      temporary.uri.resolve('build/android-arm64-$backend/install/'),
      backend,
    );
  }

  group('a directory of installs', () {
    test('serves the default backend', () async {
      build('opengl');
      build('vulkan');
      expect(
        await bundled(await runHook(_android, {'local_build': 'build'})),
        'android-arm64-opengl',
      );
    });

    test('serves the backend selected for the OS', () async {
      build('opengl');
      build('vulkan');
      final output = await runHook(_android, {
        'local_build': 'build',
        'backend': {'android': 'vulkan'},
      });
      expect(await bundled(output), 'android-arm64-vulkan');
    });

    test('rejects two installs for the same request', () async {
      build('opengl');
      _writePrefix(temporary.uri.resolve('build/copy/install/'), 'opengl');
      await expectLater(
        runHook(_android, {'local_build': 'build'}),
        throwsA(
          isStateError.having(
            (error) => error.message,
            'message',
            allOf(
              contains('several installs'),
              contains('android-arm64-opengl'),
              contains('copy'),
            ),
          ),
        ),
      );
    });
  });

  group('an install prefix', () {
    test('serves its own backend unless one is selected', () async {
      build('vulkan');
      const prefix = 'build/android-arm64-vulkan/install';
      expect(
        await bundled(await runHook(_android, {'local_build': prefix})),
        'android-arm64-vulkan',
      );
      await expectLater(
        runHook(_android, {
          'local_build': prefix,
          'backend': {'android': 'opengl'},
        }),
        throwsA(
          isStateError.having(
            (error) => error.message,
            'message',
            contains('built for android-arm64 vulkan'),
          ),
        ),
      );
    });
  });

  group('a local MinGW build', () {
    void mingw(String backend, {String architecture = 'x64'}) {
      _writePrefix(
        temporary.uri.resolve('build/mingw-$architecture-$backend/install/'),
        backend,
        platform: 'windows-mingw-$architecture',
        defaultBackend: 'vulkan',
        libraries: [
          'maplibre-native-c.dll',
          'libstdc++-6.dll',
          'libwinpthread-1.dll',
        ],
      );
    }

    for (final backend in ['opengl', 'vulkan']) {
      test('serves the $backend prefix and its runtime DLLs', () async {
        mingw(backend);
        final output = await runHook(_windows, {
          'local_build': 'build/mingw-x64-$backend/install',
        });
        expect(await bundled(output), 'windows-mingw-x64-$backend');
        expect(output.assets.code, hasLength(3));
      });
    }

    test('selects the default or requested backend in a build root', () async {
      mingw('opengl');
      mingw('vulkan');
      expect(
        await bundled(await runHook(_windows, {'local_build': 'build'})),
        'windows-mingw-x64-vulkan',
      );
      expect(
        await bundled(
          await runHook(_windows, {
            'local_build': 'build',
            'backend': {'windows': 'opengl'},
          }),
        ),
        'windows-mingw-x64-opengl',
      );
    });

    test(
      'rejects another architecture or a selected backend mismatch',
      () async {
        mingw('opengl', architecture: 'arm64');
        await expectLater(
          runHook(_windows, {
            'local_build': 'build/mingw-arm64-opengl/install',
          }),
          throwsStateError,
        );
        mingw('opengl');
        await expectLater(
          runHook(_windows, {
            'local_build': 'build/mingw-x64-opengl/install',
            'backend': {'windows': 'vulkan'},
          }),
          throwsStateError,
        );
      },
    );
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

    /// Serves an archive for each of [presets], given as platform and
    /// backend, in the release [tag] and pins them in the package. An archive
    /// is pinned under [digest] when given and its own digest otherwise.
    void release(String tag, List<(String, String)> presets, {String? digest}) {
      final platforms = <String, Map<String, Object?>>{};
      final artifacts = <Map<String, Object?>>[];
      for (final (platform, backend) in presets) {
        final root = 'maplibre-native-c-$platform-$backend';
        final archive = Archive()
          ..addFile(
            ArchiveFile.string(
              '$root/lib/libmaplibre-native-c.so',
              '$platform-$backend',
            ),
          );
        final bytes = GZipEncoder().encode(TarEncoder().encode(archive));
        served['/$tag/$root.tar.gz'] = bytes;
        platforms[platform] = {'id': platform, 'defaultBackend': 'opengl'};
        artifacts.add({
          'platform': platform,
          'backend': backend,
          'archive': '$root.tar.gz',
          'sha256': digest ?? sha256.convert(bytes).toString(),
        });
      }
      File.fromUri(temporary.uri.resolve('package/hook/native_release.json'))
        ..parent.createSync(recursive: true)
        ..writeAsStringSync(
          jsonEncode({
            'tag': tag,
            'platforms': platforms.values.toList(),
            'artifacts': artifacts,
          }),
        );
    }

    Map<String, Object?> mirror() => {
      'release_url': 'http://${server.address.host}:${server.port}/',
    };

    test('downloads its pinned archive once', () async {
      release('v0.202610.2', [
        ('android-arm64', 'opengl'),
        ('android-arm64', 'vulkan'),
      ]);
      final output = await runHook(_android, mirror());
      expect(await bundled(output), 'android-arm64-opengl');
      // The files that the build unpacked would read as changed during it.
      expect(output.dependencies, [
        File.fromUri(
          temporary.uri.resolve('package/hook/native_release.json'),
        ).absolute.uri,
      ]);

      // A later build unpacks nothing and reaches no server.
      served.clear();
      final again = await runHook(_android, mirror());
      expect(await bundled(again), 'android-arm64-opengl');
    });

    test('rejects an archive that differs from its pinned digest', () async {
      release('v0.202610.2', [('android-arm64', 'opengl')], digest: '0' * 64);
      await expectLater(runHook(_android, mirror()), throwsStateError);
    });
  });
}

/// Writes an install prefix's descriptor and [libraries], each
/// library holding the platform and backend that it stands in for, and returns
/// the files.
List<File> _writePrefix(
  Uri prefix,
  String backend, {
  String platform = 'android-arm64',
  String defaultBackend = 'opengl',
  List<String> libraries = const ['libmaplibre-native-c.so'],
}) {
  File write(String name, String content) => File.fromUri(prefix.resolve(name))
    ..parent.createSync(recursive: true)
    ..writeAsStringSync(content);
  return [
    write(
      'share/maplibre-native-c/artifact.json',
      jsonEncode({
        'platform': {'id': platform, 'defaultBackend': defaultBackend},
        'backend': backend,
      }),
    ),
    for (final library in libraries)
      write(
        '${platform.startsWith('windows-') ? 'bin' : 'lib'}/$library',
        '$platform-$backend',
      ),
  ];
}
