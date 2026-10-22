// Supplies the MapLibre Native C library as a code asset: from a local build
// that the application names, or else from the core release that this package
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
const String _releaseUrl =
    'https://github.com/maplibre/maplibre-native-ffi/releases/download';

/// The core release that a published package downloads, with the SHA-256
/// digest of each archive in it. The publish job writes this file, and git
/// ignores it.
const String _pinPath = 'hook/native_release.json';

Future<void> main(List<String> arguments) async {
  await build(arguments, (input, output) async {
    if (!input.config.buildCodeAssets) {
      return;
    }

    final code = input.config.code;
    final preset =
        '${_targetPlatform(code)}-${_backendSuffix(input, code.targetOS)}';
    final localBuild = input.userDefines.path('local_build');
    final prefix = localBuild == null
        ? await _download(input, output, preset)
        : _localPrefix(Uri.directory(localBuild.toFilePath()), preset, output);

    final library = _libraryFile(prefix, code.targetOS);
    output.dependencies.add(library.absolute.uri);
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
      output.dependencies.add(sibling.absolute.uri);
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

/// The backend half of the preset serving [os]: the application's choice, or
/// else Metal on Apple platforms, OpenGL on Android, and Vulkan elsewhere.
/// Preset names spell OpenGL by its context provider.
String _backendSuffix(BuildInput input, OS os) {
  final selected = switch (input.userDefines['backend']) {
    null => null,
    final Map<Object?, Object?> backends =>
      backends[os.name] ?? backends['default'],
    final other => throw FormatException(
      '$_defines.backend must map an OS or `default` to a backend; found '
      '$other',
    ),
  };
  return switch (selected ?? _defaultBackend(os)) {
    'opengl' => os == OS.windows ? 'wgl' : 'egl',
    final String backend when backend == 'metal' || backend == 'vulkan' =>
      backend,
    _ => throw FormatException(
      '$_defines.backend.${os.name} must be opengl, metal, or vulkan; found '
      '$selected',
    ),
  };
}

String _defaultBackend(OS os) => switch (os) {
  OS.macOS || OS.iOS => 'metal',
  OS.android => 'opengl',
  _ => 'vulkan',
};

/// Resolves a local build, which is either an install prefix or a CMake build
/// root holding an install for each preset.
Uri _localPrefix(Uri directory, String preset, BuildOutputBuilder output) {
  if (_descriptor(directory).existsSync()) {
    return directory;
  }
  final prefix = directory.resolve('$preset/install/');
  // Registered before the existence check so that building the preset later
  // reruns this hook.
  output.dependencies.add(_descriptor(prefix).absolute.uri);
  if (!_descriptor(prefix).existsSync()) {
    throw StateError(
      '$_defines.local_build names ${directory.toFilePath()}, which is neither '
      'an install prefix nor a build root holding $preset.',
    );
  }
  return prefix;
}

File _descriptor(Uri prefix) =>
    File.fromUri(prefix.resolve('share/maplibre-native-c/artifact.json'));

/// Downloads and unpacks [preset] from the pinned release, unless an earlier
/// build already did.
Future<Uri> _download(
  BuildInput input,
  BuildOutputBuilder output,
  String preset,
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
  final archiveName = 'maplibre-native-c-$preset.tar.gz';
  final digest = (pin['sha256']! as Map<String, Object?>)[archiveName];
  if (digest is! String) {
    throw UnsupportedError('$tag publishes no $archiveName.');
  }

  // The digest names the unpacked prefix, and an unpacked prefix is complete
  // because it is renamed into place.
  final prefix = Directory.fromUri(
    input.outputDirectoryShared.resolve('$digest/'),
  );
  if (prefix.existsSync()) {
    return prefix.uri;
  }

  stderr.writeln('Downloading $archiveName from $tag.');
  final archive = await _fetch('$_releaseUrl/$tag/$archiveName');
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
    // The archive holds a single `maplibre-native-c-<preset>/` root.
    Directory.fromUri(
      scratch.uri.resolve('maplibre-native-c-$preset/'),
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
    final response = await (await client.getUrl(Uri.parse(url))).close();
    if (response.statusCode != HttpStatus.ok) {
      throw HttpException('HTTP ${response.statusCode}', uri: Uri.parse(url));
    }
    final builder = BytesBuilder(copy: false);
    await response.forEach(builder.add);
    return builder.takeBytes();
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
