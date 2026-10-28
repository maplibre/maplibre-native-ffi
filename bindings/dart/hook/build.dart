// Supplies the MapLibre Native C library as a code asset: from a local build
// that the application names, or else from the release that this package
// was published against.

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:archive/archive.dart';
import 'package:code_assets/code_assets.dart';
import 'package:crypto/crypto.dart';
import 'package:hooks/hooks.dart';
import 'package:maplibre_native_ffi/src/internal/c/native_asset.dart';

const String _defines = 'hooks.user_defines.maplibre_native_ffi';
const String _defaultReleaseUrl =
    'https://github.com/maplibre/maplibre-native-ffi/releases/download';

/// The artifact index of the release that a published package downloads. The
/// release workflow writes this file, and git ignores it.
const String _pinPath = 'hook/native_release.json';

Future<void> main(List<String> arguments) async {
  await build(arguments, (input, output) async {
    if (!input.config.buildCodeAssets) {
      return;
    }

    final code = input.config.code;
    final platform = _targetPlatform(code);
    final backend = _selectedBackend(input, code.targetOS);
    final localBuild = input.userDefines.path('local_build');
    final prefix = localBuild == null
        ? await _download(input, output, platform, backend, _releaseUrl(input))
        : _localPrefix(
            Uri.directory(localBuild.toFilePath()),
            platform,
            backend,
            code.targetOS,
          );

    // A download is unpacked under its pinned digest, so the pin file, which
    // is already a dependency, covers it. Declaring the files unpacked during
    // this build would read as a change and make the runner build again.
    void dependOn(File file) {
      if (localBuild != null) {
        output.dependencies.add(file.absolute.uri);
      }
    }

    dependOn(_descriptor(prefix));
    final library = _libraryFile(prefix, code.targetOS);
    dependOn(library);
    output.assets.code.add(
      CodeAsset(
        package: input.packageName,
        name: nativeAssetName,
        linkMode: DynamicLoadingBundled(),
        file: await _bundleLibrary(library, input),
      ),
    );

    // Some presets install runtime libraries beside the C API library and load
    // them through it, resolved relative to the loader, so they have to be
    // declared as assets too.
    for (final sibling in _siblingLibraries(library, code.targetOS)) {
      dependOn(sibling);
      output.assets.code.add(
        CodeAsset(
          package: input.packageName,
          name: 'native/${sibling.uri.pathSegments.last}',
          linkMode: DynamicLoadingBundled(),
          file: await _bundleLibrary(sibling, input),
        ),
      );
    }
  });
}

/// Names the platform half of the preset serving [code].
String _targetPlatform(CodeConfig code) {
  final architecture = switch (code.targetArchitecture) {
    Architecture.x64 => 'x64',
    Architecture.arm64 => 'arm64',
    final other => other.toString(),
  };
  return switch (code.targetOS) {
    OS.linux => 'linux-gnu-$architecture',
    OS.iOS when code.iOS.targetSdk == IOSSdk.iPhoneSimulator =>
      'ios-simulator-$architecture',
    final os => '$os-$architecture',
  };
}

/// The backend that the application selected for [os], if it selected one.
String? _selectedBackend(BuildInput input, OS os) =>
    switch (input.userDefines['backend']) {
      null => null,
      final Map<Object?, Object?> backends => switch (backends[os.name]) {
        null => null,
        final String backend
            when backend == 'opengl' ||
                backend == 'metal' ||
                backend == 'vulkan' =>
          backend,
        final other => throw FormatException(
          '$_defines.backend.${os.name} must be opengl, metal, or vulkan; '
          'found $other',
        ),
      },
      final other => throw FormatException(
        '$_defines.backend must map an OS to a backend; found $other',
      ),
    };

/// The base URL of the release downloads: the GitHub releases, or else a mirror
/// that the application names, which serves each release tag as a directory.
String _releaseUrl(BuildInput input) =>
    switch (input.userDefines['release_url']) {
      null => _defaultReleaseUrl,
      final String url =>
        url.endsWith('/') ? url.substring(0, url.length - 1) : url,
      final other => throw FormatException(
        '$_defines.release_url must be a URL; found $other',
      ),
    };

/// Resolves a local build: an install prefix, or a directory holding installs
/// one level down, such as a CMake build root.
Uri _localPrefix(Uri directory, String platform, String? backend, OS os) {
  final request = _request(platform, backend);
  if (_descriptor(directory).existsSync()) {
    // A prefix serves one platform, so a build for several targets names a
    // directory of installs instead. Only a selected backend can conflict;
    // without one, the prefix's own backend serves.
    final descriptor = _readDescriptor(directory);
    final built = _platform(descriptor);
    if (built['id'] != platform ||
        (backend != null && descriptor['backend'] != backend)) {
      throw StateError(
        '$_defines.local_build names an install prefix built for '
        '${built['id']} ${descriptor['backend']}, '
        'which cannot serve $request.',
      );
    }
    return directory;
  }

  // Without a selected backend, only the target's default backend serves, so
  // that building another backend under the same directory never changes
  // which install an unchanged request gets.
  final root = Directory.fromUri(directory);
  final matches = [
    if (root.existsSync())
      for (final child in root.listSync().whereType<Directory>())
        for (final prefix in [child.uri.resolve('install/'), child.uri])
          if (_descriptor(prefix).existsSync() &&
              _serves(_readDescriptor(prefix), platform, backend))
            prefix,
  ]..sort((a, b) => a.path.compareTo(b.path));
  final named =
      '$_defines.local_build names ${directory.toFilePath()}, which holds';
  if (matches.length > 1) {
    throw StateError(
      '$named several installs for $request:'
      '${matches.map((prefix) => '\n  ${prefix.toFilePath()}').join()}\n'
      'Remove all but one, or name one of them as the install prefix.',
    );
  }
  if (matches.isEmpty) {
    throw StateError(
      '$named no install for $request. Build it, or select another backend '
      'in $_defines.backend.${os.name}.',
    );
  }
  return matches.single;
}

/// Describes a request for [platform] with the [backend] selected for it.
String _request(String platform, String? backend) => backend == null
    ? '$platform with its default backend'
    : '$platform $backend';

/// Whether [descriptor] serves [platform] with the [backend] selected for it,
/// or without one, with the platform's default backend.
bool _serves(
  Map<String, Object?> descriptor,
  String platform,
  String? backend,
) {
  final built = _platform(descriptor);
  return built['id'] == platform &&
      descriptor['backend'] == (backend ?? built['defaultBackend']);
}

Map<String, Object?> _platform(Map<String, Object?> descriptor) =>
    descriptor['platform']! as Map<String, Object?>;

Map<String, Object?> _readDescriptor(Uri prefix) =>
    jsonDecode(_descriptor(prefix).readAsStringSync()) as Map<String, Object?>;

File _descriptor(Uri prefix) =>
    File.fromUri(prefix.resolve('share/maplibre-native-c/artifact.json'));

/// Downloads and unpacks the pinned release's archive for [platform] with the
/// [backend] selected for it, unless an earlier build already did.
Future<Uri> _download(
  BuildInput input,
  BuildOutputBuilder output,
  String platform,
  String? backend,
  String releaseUrl,
) async {
  final pinFile = File.fromUri(input.packageRoot.resolve(_pinPath));
  if (!pinFile.existsSync()) {
    throw StateError(
      'Only a published maplibre_native_ffi package downloads the native '
      'library. Name a local build in $_defines.local_build.',
    );
  }
  output.dependencies.add(pinFile.absolute.uri);
  final pin = jsonDecode(pinFile.readAsStringSync()) as Map<String, Object?>;
  final tag = pin['tag']! as String;
  final published = (pin['platforms']! as List<Object?>)
      .cast<Map<String, Object?>>()
      .where((entry) => entry['id'] == platform)
      .firstOrNull;
  final wanted = backend ?? published?['defaultBackend'];
  final entries = [
    if (published != null)
      for (final entry
          in (pin['artifacts']! as List<Object?>).cast<Map<String, Object?>>())
        if (entry['platform'] == platform && entry['backend'] == wanted) entry,
  ];
  if (entries.length != 1) {
    throw UnsupportedError(
      '$tag publishes ${entries.isEmpty ? 'no' : entries.length} archives '
      'for ${_request(platform, backend)}; expected one.',
    );
  }
  final archiveName = entries.single['archive']! as String;
  final digest = entries.single['sha256']! as String;

  // The digest names the unpacked prefix, and an unpacked prefix is complete
  // because it is renamed into place.
  final prefix = Directory.fromUri(
    input.outputDirectoryShared.resolve('$digest/'),
  );
  if (prefix.existsSync()) {
    return prefix.uri;
  }

  stderr.writeln('Downloading $archiveName from $tag.');
  final archive = await _fetch('$releaseUrl/$tag/$archiveName');
  if (sha256.convert(archive).toString() != digest) {
    throw StateError('$archiveName does not match its digest pinned for $tag.');
  }

  final scratch = Directory.fromUri(
    input.outputDirectoryShared.resolve('$digest.$pid/'),
  );
  try {
    for (final entry in TarDecoder().decodeBytes(
      GZipDecoder().decodeBytes(archive),
    )) {
      if (entry.isFile) {
        File.fromUri(scratch.uri.resolve(entry.name))
          ..parent.createSync(recursive: true)
          ..writeAsBytesSync(entry.content);
      }
    }
    // The archive holds a single root named after it.
    Directory.fromUri(
      scratch.uri.resolve(
        '${archiveName.replaceFirst(RegExp(r'\.tar\.gz$'), '')}/',
      ),
    ).renameSync(prefix.path);
  } on FileSystemException {
    // A concurrent build that unpacked the same digest first leaves the same
    // tree in place.
    if (!prefix.existsSync()) {
      rethrow;
    }
  } finally {
    if (scratch.existsSync()) {
      scratch.deleteSync(recursive: true);
    }
  }
  return prefix.uri;
}

Future<Uint8List> _fetch(String url) async {
  final client = HttpClient()
    ..connectionTimeout = const Duration(seconds: 30)
    // Hooks run with a stripped environment, but HTTP_PROXY, HTTPS_PROXY, and
    // NO_PROXY are among the variables passed through.
    ..findProxy = HttpClient.findProxyFromEnvironment;
  try {
    // The connection timeout ends at the response headers, so this bound
    // covers a body that stalls. It allows for a slow link pulling a 30 MB
    // archive.
    return await () async {
      final response = await (await client.getUrl(Uri.parse(url))).close();
      if (response.statusCode != HttpStatus.ok) {
        throw HttpException('HTTP ${response.statusCode}', uri: Uri.parse(url));
      }
      final builder = BytesBuilder(copy: false);
      await response.forEach(builder.add);
      return builder.takeBytes();
    }().timeout(const Duration(minutes: 10));
  } finally {
    client.close(force: true);
  }
}

/// Resolves the shared library within an install prefix.
///
/// The library directory is `lib` on most platforms, `lib64` where the
/// toolchain's `CMAKE_INSTALL_LIBDIR` says so, and `bin` on Windows.
File _libraryFile(Uri prefix, OS targetOS) {
  final fileName = switch (targetOS) {
    OS.macOS || OS.iOS => 'libmaplibre-native-c.dylib',
    OS.windows => 'maplibre-native-c.dll',
    _ => 'libmaplibre-native-c.so',
  };
  final directories = targetOS == OS.windows
      ? const ['bin']
      : const ['lib', 'lib64'];
  for (final directory in directories) {
    final candidate = File.fromUri(prefix.resolve('$directory/$fileName'));
    if (candidate.existsSync()) {
      return candidate;
    }
  }
  throw FileSystemException(
    'Install prefix holds no MapLibre Native C library',
    prefix.toFilePath(),
  );
}

/// Lists the other shared libraries installed beside [library], leaving out the
/// test graphics fixtures that a build tree's install also holds.
Iterable<File> _siblingLibraries(File library, OS targetOS) {
  final extension = switch (targetOS) {
    OS.macOS || OS.iOS => '.dylib',
    OS.windows => '.dll',
    _ => '.so',
  };
  return library.parent.listSync().whereType<File>().where(
    (file) =>
        file.path != library.path &&
        file.path.endsWith(extension) &&
        !file.uri.pathSegments.last.contains('mln_test_graphics'),
  );
}

// Flutter treats declared assets as generated outputs and deletes stale ones.
// Keep those outputs separate from shared installs and downloaded artifacts.
Future<Uri> _bundleLibrary(File library, BuildInput input) async {
  final destination = input.outputDirectory.resolve(
    library.uri.pathSegments.last,
  );
  return (await library.copy(destination.toFilePath())).uri;
}
