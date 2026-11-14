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
    for (final name in ['first', 'second']) {
      sources.addAll(
        _writePrefix(
          temporary.uri.resolve('$name/'),
          label: name,
          libraries: ['libmaplibre-native-c.so', 'libdependency.so'],
        ),
      );
      final output = await runHook(_android, {'local_build': name});
      expect(output.assets.code, hasLength(2));
      for (final asset in output.assets.code) {
        final file = File.fromUri(asset.file!);
        expect(await file.readAsString(), name);
        // Build tools may remove any declared output when configurations change.
        await file.delete();
      }
      for (final source in sources) {
        expect(source.existsSync(), isTrue, reason: source.path);
      }
    }
  });

  /// Reads the first library that [output] declares, which names the install
  /// that it came from.
  Future<String> bundled(BuildOutput output) =>
      File.fromUri(output.assets.code.first.file!).readAsString();

  /// Writes an Android arm64 install named [name] under the build root, in the
  /// CMake layout, that carries [backends].
  void build(String name, List<String> backends) {
    _writePrefix(
      temporary.uri.resolve('build/$name/install/'),
      label: name,
      backends: backends,
    );
  }

  group('a directory of installs', () {
    test('serves the install that carries the default backend', () async {
      build('gl', ['opengl']);
      build('vk', ['vulkan']);
      expect(
        await bundled(await runHook(_android, {'local_build': 'build'})),
        'gl',
      );
    });

    test(
      'serves the install that carries the backend selected for the OS',
      () async {
        build('gl', ['opengl']);
        build('vk', ['vulkan']);
        final output = await runHook(_android, {
          'local_build': 'build',
          'backend': {'android': 'vulkan'},
        });
        expect(await bundled(output), 'vk');
      },
    );

    test('rejects two installs for the same request', () async {
      build('android-arm64', ['opengl', 'vulkan']);
      build('copy', ['opengl']);
      await expectLater(
        runHook(_android, {'local_build': 'build'}),
        throwsA(
          isStateError.having(
            (error) => error.message,
            'message',
            allOf(
              contains('several installs'),
              contains('android-arm64 with its default backend'),
              contains('copy'),
            ),
          ),
        ),
      );
    });

    test('rejects a selected backend that no install carries', () async {
      build('android-arm64', ['opengl', 'vulkan']);
      await expectLater(
        runHook(_android, {
          'local_build': 'build',
          'backend': {'android': 'metal'},
        }),
        throwsA(
          isStateError.having(
            (error) => error.message,
            'message',
            contains('no install for android-arm64 with metal'),
          ),
        ),
      );
    });
  });

  group('an install prefix', () {
    test('serves unless it lacks the selected backend', () async {
      build('android-arm64', ['opengl', 'vulkan']);
      const prefix = 'build/android-arm64/install';
      expect(
        await bundled(await runHook(_android, {'local_build': prefix})),
        'android-arm64',
      );
      expect(
        await bundled(
          await runHook(_android, {
            'local_build': prefix,
            'backend': {'android': 'vulkan'},
          }),
        ),
        'android-arm64',
      );
      await expectLater(
        runHook(_android, {
          'local_build': prefix,
          'backend': {'android': 'metal'},
        }),
        throwsA(
          isStateError.having(
            (error) => error.message,
            'message',
            contains('built for android-arm64 with opengl, vulkan'),
          ),
        ),
      );
    });
  });

  group('a local MinGW build', () {
    void mingw(String architecture) {
      _writePrefix(
        temporary.uri.resolve('build/mingw-$architecture/install/'),
        platform: 'windows-mingw-$architecture',
        backends: ['vulkan', 'opengl'],
        defaultBackend: 'vulkan',
        libraries: [
          'maplibre-native-c.dll',
          if (architecture == 'arm64') ...[
            'libc++.dll',
            'libunwind.dll',
          ] else ...[
            'libgcc_s_seh-1.dll',
            'libstdc++-6.dll',
            'libwinpthread-1.dll',
          ],
        ],
      );
    }

    for (final architecture in [Architecture.x64, Architecture.arm64]) {
      test('serves the $architecture prefix and its runtime DLLs', () async {
        final arch = architecture == Architecture.arm64 ? 'arm64' : 'x64';
        mingw(arch);
        final output = await runHook(
          CodeAssetExtension(
            targetArchitecture: architecture,
            targetOS: OS.windows,
            linkModePreference: LinkModePreference.dynamic,
          ),
          {'local_build': 'build/mingw-$arch/install'},
        );
        expect(await bundled(output), 'windows-mingw-$arch');
        expect(output.assets.code, hasLength(arch == 'arm64' ? 3 : 4));
      });
    }

    test(
      'serves the default or a requested backend from a build root',
      () async {
        mingw('x64');
        expect(
          await bundled(await runHook(_windows, {'local_build': 'build'})),
          'windows-mingw-x64',
        );
        expect(
          await bundled(
            await runHook(_windows, {
              'local_build': 'build',
              'backend': {'windows': 'opengl'},
            }),
          ),
          'windows-mingw-x64',
        );
      },
    );

    test('rejects another architecture or a backend it lacks', () async {
      mingw('arm64');
      await expectLater(
        runHook(_windows, {'local_build': 'build/mingw-arm64/install'}),
        throwsStateError,
      );
      mingw('x64');
      await expectLater(
        runHook(_windows, {
          'local_build': 'build/mingw-x64/install',
          'backend': {'windows': 'metal'},
        }),
        throwsStateError,
      );
    });
  });

  test('serves a local MSVC build and its Visual C++ runtime DLLs', () async {
    _writePrefix(
      temporary.uri.resolve('build/windows-x64/install/'),
      platform: 'windows-x64',
      backends: ['vulkan', 'opengl'],
      defaultBackend: 'vulkan',
      libraries: ['maplibre-native-c.dll', 'msvcp140.dll', 'vcruntime140.dll'],
    );
    final output = await runHook(_windows, {
      'local_build': 'build/windows-x64/install',
    });
    expect(await bundled(output), 'windows-x64');
    expect(output.assets.code, hasLength(3));
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

    /// Serves an archive for each of [archives], given as the archive's name,
    /// its platform, and the backends it carries, in the release [tag] and
    /// pins them in the package. An archive is pinned under [digest] when
    /// given and its own digest otherwise.
    void release(
      String tag,
      List<(String, String, List<String>)> archives, {
      String? digest,
    }) {
      final platforms = <String, Map<String, Object?>>{};
      final artifacts = <Map<String, Object?>>[];
      for (final (name, platform, backends) in archives) {
        final root = 'maplibre-native-c-$name';
        final archive = Archive()
          ..addFile(
            ArchiveFile.string('$root/lib/libmaplibre-native-c.so', name),
          );
        final bytes = GZipEncoder().encode(TarEncoder().encode(archive));
        served['/$tag/$root.tar.gz'] = bytes;
        platforms[platform] = {'id': platform, 'defaultBackend': 'opengl'};
        artifacts.add({
          'platform': platform,
          'backends': backends,
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
        ('android-arm64', 'android-arm64', ['opengl', 'vulkan']),
        ('android-x64', 'android-x64', ['opengl', 'vulkan']),
      ]);
      final output = await runHook(_android, mirror());
      expect(await bundled(output), 'android-arm64');
      // The files that the build unpacked would read as changed during it.
      expect(output.dependencies, [
        File.fromUri(
          temporary.uri.resolve('package/hook/native_release.json'),
        ).absolute.uri,
      ]);

      // A later build unpacks nothing and reaches no server.
      served.clear();
      final again = await runHook(_android, mirror());
      expect(await bundled(again), 'android-arm64');
    });

    test('downloads the archive that carries the selected backend', () async {
      release('v0.202610.2', [
        ('gl', 'android-arm64', ['opengl']),
        ('vk', 'android-arm64', ['vulkan']),
      ]);
      expect(await bundled(await runHook(_android, mirror())), 'gl');
      expect(
        await bundled(
          await runHook(_android, {
            ...mirror(),
            'backend': {'android': 'vulkan'},
          }),
        ),
        'vk',
      );
      await expectLater(
        runHook(_android, {
          ...mirror(),
          'backend': {'android': 'metal'},
        }),
        throwsUnsupportedError,
      );
    });

    test('rejects an archive that differs from its pinned digest', () async {
      release('v0.202610.2', [
        ('android-arm64', 'android-arm64', ['opengl', 'vulkan']),
      ], digest: '0' * 64);
      await expectLater(runHook(_android, mirror()), throwsStateError);
    });
  });
}

/// Writes an install prefix's descriptor and [libraries], each library holding
/// [label], or else the platform, to name the install that it came from, and
/// returns the files.
List<File> _writePrefix(
  Uri prefix, {
  String? label,
  String platform = 'android-arm64',
  List<String> backends = const ['opengl', 'vulkan'],
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
        'backends': backends,
      }),
    ),
    // An MSVC install carries its runtime DLLs beside the library in bin, and
    // a MinGW install in lib.
    for (final library in libraries)
      write(
        '${platform.startsWith('windows-') && (library == 'maplibre-native-c.dll' || !platform.startsWith('windows-mingw-')) ? 'bin' : 'lib'}/$library',
        label ?? platform,
      ),
  ];
}
