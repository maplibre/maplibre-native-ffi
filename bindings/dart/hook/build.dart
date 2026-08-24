// Supplies the MapLibre Native C library as a code asset, from a local build
// when one is configured and otherwise from the published snapshot release.

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:archive/archive.dart';
import 'package:code_assets/code_assets.dart';
import 'package:crypto/crypto.dart';
import 'package:hooks/hooks.dart';
import 'package:maplibre_native_ffi/src/internal/c/native_asset.dart';

/// File naming the install prefix of a native library built from this
/// repository, written by the mise tasks before they invoke Dart.
///
/// Build hooks run with a stripped environment, so the prefix arrives in a file
/// rather than in `MLN_FFI_NATIVE_LIBRARY`.
const String installPrefixPointer = '.dart_tool/maplibre_native_install_dir';

const String _libraryName = 'maplibre-native-c';
const String _snapshotTag = 'unstable-native-snapshot';
const String _releaseBaseUrl =
    'https://github.com/maplibre/maplibre-native-ffi/releases/download';

void main(List<String> arguments) async {
  await build(arguments, (input, output) async {
    if (!input.config.buildCodeAssets) {
      return;
    }

    final library = _libraryFile(
      await _installPrefix(input, output),
      input.config.code,
    );
    output.dependencies.add(library.absolute.uri);
    output.assets.code.add(
      CodeAsset(
        package: input.packageName,
        name: nativeAssetName,
        linkMode: DynamicLoadingBundled(),
        file: library.absolute.uri,
      ),
    );

    // Some presets install runtime libraries beside the C API library and load
    // them through it, resolved relative to the loader, so they have to be
    // declared as assets too.
    for (final sibling in _siblingLibraries(
      library,
      input.config.code.targetOS,
    )) {
      output.dependencies.add(sibling.absolute.uri);
      output.assets.code.add(
        CodeAsset(
          package: input.packageName,
          name: 'native/${sibling.uri.pathSegments.last}',
          linkMode: DynamicLoadingBundled(),
          file: sibling.absolute.uri,
        ),
      );
    }
  });
}

/// Rejects an install prefix that does not match what this build asked for.
///
/// The pointer names whichever preset was built last, and a library file name
/// alone does not distinguish one target from another, so the artifact
/// descriptor decides.
void _verifyLocalPrefix(Uri prefix, BuildInput input) {
  final descriptor = _descriptor(prefix);
  final expected = _targetPlatform(input.config.code);
  final actual = descriptor['targetPlatform'];
  if (actual != expected) {
    throw StateError(
      'Install prefix was built for $actual, but this build targets '
      '$expected. Build that preset and point $installPrefixPointer at it.',
    );
  }

  // Only an explicit request conflicts; absent one, the prefix's own renderer
  // is the answer rather than the download path's platform default.
  final backend = _requestedBackend(input);
  if (backend != null && descriptor['renderBackend'] != backend) {
    throw StateError(
      'Install prefix holds a ${descriptor['renderBackend']} artifact, but '
      'this application selected $backend. Build that preset and point '
      '$installPrefixPointer at it, or drop the backend user-define.',
    );
  }
}

/// The render backend the root application selected, if it selected one.
String? _requestedBackend(BuildInput input) {
  final requested = input.userDefines['backend'];
  if (requested == null) {
    return null;
  }
  if (requested is! String) {
    throw const FormatException(
      'hooks.user_defines.maplibre_native_ffi.backend must be a string',
    );
  }
  return requested;
}

/// Names the platform half of the preset serving [code], as the descriptor's
/// `targetPlatform` spells it.
String _targetPlatform(CodeConfig code) {
  final architecture = switch (code.targetArchitecture) {
    Architecture.x64 => 'x64',
    Architecture.arm64 => 'arm64',
    final other => other.toString(),
  };
  return switch (code.targetOS) {
    OS.linux => 'linux-gnu-$architecture',
    OS.macOS => 'macos-$architecture',
    OS.windows => 'windows-$architecture',
    OS.android => 'android-$architecture',
    OS.iOS =>
      code.iOS.targetSdk == IOSSdk.iPhoneSimulator
          ? 'ios-simulator-$architecture'
          : 'ios-$architecture',
    final other => '$other-$architecture',
  };
}

/// Lists the other shared libraries installed beside [library].
Iterable<File> _siblingLibraries(File library, OS targetOS) {
  final extension = switch (targetOS) {
    OS.macOS || OS.iOS => '.dylib',
    OS.windows => '.dll',
    _ => '.so',
  };
  return library.parent.listSync().whereType<File>().where(
    (file) => file.path != library.path && file.path.endsWith(extension),
  );
}

/// Resolves an install prefix, preferring a local build over a download.
Future<Uri> _installPrefix(BuildInput input, BuildOutputBuilder output) async {
  final pointer = File.fromUri(input.packageRoot.resolve(installPrefixPointer));
  // Registered before the existence check so that writing the file later reruns
  // this hook.
  output.dependencies.add(pointer.absolute.uri);
  if (pointer.existsSync()) {
    // A pointer that names an unusable prefix fails rather than downloading:
    // the pointer is an explicit opt-out of the network.
    final prefix = Uri.directory(pointer.readAsStringSync().trim());
    _verifyLocalPrefix(prefix, input);
    _libraryFile(prefix, input.config.code);
    return prefix;
  }

  final preset = _resolvePreset(input);
  final prefix = await _downloadPrefix(input, output, preset);
  _verifyDescriptor(prefix, preset);
  _warnOnHeaderSkew(input, output, prefix, preset);
  return prefix;
}

/// A published `<os>-<arch>-<backend>` artifact.
final class _Preset {
  const _Preset(this.name, this.platform, this.backend);

  /// Full preset name, as used in the asset file name.
  final String name;

  /// Platform half, matching the descriptor's `targetPlatform`.
  final String platform;

  /// Render backend selector, matching the descriptor's `renderBackend`.
  /// Spelled `opengl` even where the preset suffix names the context provider.
  final String backend;
}

/// Backend selectors paired with the preset suffix each maps to. Preset names
/// spell the OpenGL backend by its context provider.
const Map<String, String> _openglEgl = {'vulkan': 'vulkan', 'opengl': 'egl'};
const Map<String, String> _openglWgl = {'vulkan': 'vulkan', 'opengl': 'wgl'};
const Map<String, String> _appleDesktop = {
  'metal': 'metal',
  'vulkan': 'vulkan',
  'opengl': 'egl',
};
const Map<String, String> _appleMobile = {'metal': 'metal'};

/// The presets the snapshot release publishes a shared library for, keyed by
/// the target they serve. The release also carries OpenHarmony archives, which
/// this table leaves out because `OS` defines no OpenHarmony value for a build
/// to arrive with.
const Map<String, ({String defaultBackend, Map<String, String> backends})>
_platformTargets = {
  'linux-gnu-x64': (defaultBackend: 'vulkan', backends: _openglEgl),
  'linux-gnu-arm64': (defaultBackend: 'vulkan', backends: _openglEgl),
  'macos-arm64': (defaultBackend: 'metal', backends: _appleDesktop),
  'windows-x64': (defaultBackend: 'vulkan', backends: _openglWgl),
  'windows-arm64': (defaultBackend: 'vulkan', backends: _openglWgl),
  'android-arm': (defaultBackend: 'opengl', backends: _openglEgl),
  'android-arm64': (defaultBackend: 'opengl', backends: _openglEgl),
  'android-x64': (defaultBackend: 'opengl', backends: _openglEgl),
  'ios-arm64': (defaultBackend: 'metal', backends: _appleMobile),
  'ios-simulator-arm64': (defaultBackend: 'metal', backends: _appleMobile),
};

_Preset _resolvePreset(BuildInput input) {
  final code = input.config.code;
  final platform = _targetPlatform(code);
  final target = _platformTargets[platform];
  if (target == null) {
    throw UnsupportedError(
      'No native artifact is published for ${code.targetOS} '
      '${code.targetArchitecture}. Build the native library from this '
      'repository and write its install prefix to $installPrefixPointer.',
    );
  }

  final backend = _requestedBackend(input) ?? target.defaultBackend;
  final suffix = target.backends[backend];
  if (suffix == null) {
    throw UnsupportedError(
      'The $backend backend is not built for $platform; MapLibre Native '
      'compiles one renderer per artifact, and $platform offers '
      '${target.backends.keys.join(', ')}.',
    );
  }
  return _Preset('$platform-$suffix', platform, backend);
}

/// Downloads and extracts the archive for [preset] unless it is already cached.
///
/// The snapshot release is floating, so its asset URLs never change and the
/// `SHA256SUMS` digest is the cache key instead.
///
/// A build that starts mid-publish can pair one generation's checksum with
/// another's bytes, so a mismatch is retried once against a freshly fetched
/// checksum file and is fatal after that. The retry takes no cached answer,
/// neither the offline fallback nor a cache hit.
Future<Uri> _downloadPrefix(
  BuildInput input,
  BuildOutputBuilder output,
  _Preset preset, {
  bool afterMismatch = false,
}) async {
  final releaseBaseUrl = _snapshotBaseUrl(input, output);
  final cacheDirectory = Directory.fromUri(
    input.outputDirectoryShared.resolve('${preset.name}/'),
  )..createSync(recursive: true);

  final String checksums;
  try {
    checksums = utf8.decode(
      await _fetch('$releaseBaseUrl/$_snapshotTag/SHA256SUMS'),
    );
  } on Object catch (error) {
    if (afterMismatch) {
      rethrow;
    }
    return _reuseCached(cacheDirectory, preset, error);
  }

  final prefix = Directory.fromUri(
    cacheDirectory.uri.resolve('${sha256.convert(utf8.encode(checksums))}/'),
  );
  if (prefix.existsSync() && !afterMismatch) {
    return prefix.uri;
  }

  final archiveName = '$_libraryName-${preset.name}.tar.gz';
  final expected = _checksumFor(checksums, archiveName);
  stderr.writeln('Downloading $archiveName from the $_snapshotTag release.');
  final Uint8List archive;
  try {
    archive = await _fetch('$releaseBaseUrl/$_snapshotTag/$archiveName');
  } on Object catch (error) {
    // A publish race can list an archive in SHA256SUMS before uploading it.
    // Only the fetch falls back; the checksum check below stays fatal.
    if (afterMismatch) {
      rethrow;
    }
    return _reuseCached(cacheDirectory, preset, error);
  }

  final actual = sha256.convert(archive).toString();
  if (actual != expected) {
    if (!afterMismatch) {
      stderr.writeln(
        'Checksum mismatch for $archiveName; the $_snapshotTag release may '
        'have been republished mid-download. Retrying once.',
      );
      return _downloadPrefix(input, output, preset, afterMismatch: true);
    }
    throw StateError(
      'Checksum mismatch for $archiveName: expected $expected, got $actual.',
    );
  }

  _extract(archive, cacheDirectory, prefix, preset, archiveName);
  return prefix.uri;
}

String _snapshotBaseUrl(BuildInput input, BuildOutputBuilder output) {
  final pointer = input.userDefines.path('test_snapshot_base_url_file');
  if (pointer == null) {
    return _releaseBaseUrl;
  }
  output.dependencies.add(pointer);
  final file = File.fromUri(pointer);
  if (!file.existsSync()) {
    return _releaseBaseUrl;
  }
  return file.readAsStringSync().trim();
}

/// Unpacks into a scratch directory and moves the result into place, so a
/// failed extraction leaves no usable cache entry and a concurrent build never
/// observes a partial one.
void _extract(
  Uint8List archive,
  Directory cacheDirectory,
  Directory prefix,
  _Preset preset,
  String archiveName,
) {
  final scratch = Directory.fromUri(
    cacheDirectory.uri.resolve('.extract-$pid/'),
  );
  if (scratch.existsSync()) {
    scratch.deleteSync(recursive: true);
  }
  scratch.createSync(recursive: true);

  try {
    // The archive holds a single `maplibre-native-c-<preset>/` root, so
    // unpacking into the scratch directory yields the prefix as a child.
    final root = '$_libraryName-${preset.name}/';
    for (final entry in TarDecoder().decodeBytes(
      GZipDecoder().decodeBytes(archive),
    )) {
      if (!entry.isFile || !entry.name.startsWith(root)) {
        continue;
      }
      // `Uri.resolve` collapses `..`, so a name that starts with the expected
      // root can still land outside the scratch tree. The comparison is on
      // resolved filesystem paths, and requires a trailing separator, so a
      // Windows backslash and a sibling directory sharing the scratch name as
      // a prefix are both rejected.
      final target = scratch.uri.resolve(entry.name);
      final targetPath = File.fromUri(target).absolute.path;
      final scratchPath = scratch.absolute.path.endsWith(Platform.pathSeparator)
          ? scratch.absolute.path
          : '${scratch.absolute.path}${Platform.pathSeparator}';
      if (!targetPath.startsWith(scratchPath)) {
        throw StateError(
          'Refusing $archiveName: entry "${entry.name}" escapes the archive '
          'root.',
        );
      }
      final file = File(targetPath);
      file.parent.createSync(recursive: true);
      file.writeAsBytesSync(entry.content);
    }

    try {
      Directory.fromUri(scratch.uri.resolve(root)).renameSync(prefix.path);
    } on FileSystemException {
      // Losing the rename means another build extracted the same digest first,
      // which is the same tree by construction.
      if (!prefix.existsSync()) {
        rethrow;
      }
    }
  } finally {
    if (scratch.existsSync()) {
      scratch.deleteSync(recursive: true);
    }
  }
}

/// Falls back to a previously downloaded prefix when the release is
/// unreachable, so offline builds keep working.
Uri _reuseCached(Directory cacheDirectory, _Preset preset, Object error) {
  final cached =
      cacheDirectory
          .listSync()
          .whereType<Directory>()
          .where(
            (directory) => Directory.fromUri(
              directory.uri.resolve('include/'),
            ).existsSync(),
          )
          .toList()
        ..sort(
          (a, b) => a.statSync().modified.compareTo(b.statSync().modified),
        );

  if (cached.isEmpty) {
    throw StateError(
      'Could not reach the native snapshot release ($error) and no cached '
      '${preset.name} artifact is available. Build the native library from '
      'this repository and write its install prefix to $installPrefixPointer '
      'to build without network access.',
    );
  }
  stderr.writeln(
    'Could not reach the native snapshot release ($error); reusing the cached '
    '${preset.name} artifact, which may be out of date.',
  );
  return cached.last.uri;
}

/// Bounds every request so a stalled host fails into the cache fallback rather
/// than hanging the build. The request bound covers the body too, so it allows
/// for a slow link pulling a 30 MB archive.
const Duration _connectTimeout = Duration(seconds: 30);
const Duration _requestTimeout = Duration(minutes: 10);

Future<Uint8List> _fetch(String url) async {
  final client = HttpClient()..connectionTimeout = _connectTimeout;
  try {
    // Hooks run with a stripped environment, but HTTP_PROXY, HTTPS_PROXY, and
    // NO_PROXY are among the variables passed through, so honour them.
    client.findProxy = HttpClient.findProxyFromEnvironment;
    return await () async {
      final request = await client.getUrl(Uri.parse(url));
      final response = await request.close();
      if (response.statusCode != HttpStatus.ok) {
        throw HttpException('HTTP ${response.statusCode}', uri: Uri.parse(url));
      }
      final builder = BytesBuilder(copy: false);
      await response.forEach(builder.add);
      return builder.takeBytes();
    }().timeout(_requestTimeout);
  } finally {
    client.close(force: true);
  }
}

String _checksumFor(String checksums, String archiveName) {
  for (final line in const LineSplitter().convert(checksums)) {
    final parts = line.split('  ');
    if (parts.length == 2 && parts[1] == archiveName) {
      return parts[0];
    }
  }
  throw StateError('The native snapshot release has no $archiveName entry.');
}

void _verifyDescriptor(Uri prefix, _Preset preset) {
  final descriptor = _descriptor(prefix);
  if (descriptor['targetPlatform'] != preset.platform ||
      descriptor['renderBackend'] != preset.backend) {
    throw StateError(
      '${prefix.toFilePath()} holds a ${descriptor['targetPlatform']}/'
      '${descriptor['renderBackend']} artifact but ${preset.name} was '
      'requested.',
    );
  }
}

/// Reads the artifact descriptor recording what a prefix was built for.
Map<String, Object?> _descriptor(Uri prefix) {
  final file = File.fromUri(
    prefix.resolve('share/$_libraryName/artifact.json'),
  );
  if (!file.existsSync()) {
    throw FileSystemException(
      'Install prefix holds no $_libraryName descriptor. Rebuild it with '
      '`mise run build`, or delete $installPrefixPointer to stop pointing at '
      'it',
      file.path,
    );
  }
  return jsonDecode(file.readAsStringSync()) as Map<String, Object?>;
}

/// Compares the checkout's public headers against the downloaded artifact's.
///
/// The artifact's commit lags this checkout by design, so differing headers
/// rather than differing commits are the condition worth warning about.
void _warnOnHeaderSkew(
  BuildInput input,
  BuildOutputBuilder output,
  Uri prefix,
  _Preset preset,
) {
  final checkoutInclude = input.packageRoot.resolve('../../include/');
  final checkout = _publicHeaders(checkoutInclude);
  final artifact = _publicHeaders(prefix.resolve('include/'));
  if (checkout == null || artifact == null) {
    return;
  }

  // Editing a public header in a path dependency must rerun this hook.
  for (final name in checkout.keys) {
    output.dependencies.add(checkoutInclude.resolve(name));
  }

  final differing = {
    ...checkout.keys,
    ...artifact.keys,
  }.where((name) => checkout[name] != artifact[name]).toList()..sort();
  if (differing.isEmpty) {
    return;
  }

  final gitSha = _descriptor(prefix)['gitSha'];
  stderr.writeln(
    'The downloaded ${preset.name} artifact'
    '${gitSha == null ? '' : ' (built from $gitSha)'} does not match this '
    "checkout's C headers: ${differing.join(', ')}. The snapshot release "
    'publishes on its own schedule, so it can lag this commit; build the '
    'native library from source if the difference matters.',
  );
}

/// Digests the public C headers, keyed by their path under `include/`. Render
/// backend dependencies install their own headers alongside ours, so this
/// covers only the umbrella header and its domain directory.
Map<String, String>? _publicHeaders(Uri includeDirectory) {
  final umbrella = File.fromUri(
    includeDirectory.resolve('maplibre_native_c.h'),
  );
  if (!umbrella.existsSync()) {
    return null;
  }
  final headers = {
    'maplibre_native_c.h': sha256
        .convert(umbrella.readAsBytesSync())
        .toString(),
  };
  final domain = Directory.fromUri(
    includeDirectory.resolve('maplibre_native_c/'),
  );
  if (domain.existsSync()) {
    for (final entry in domain.listSync(recursive: true).whereType<File>()) {
      if (!entry.path.endsWith('.h')) {
        continue;
      }
      final name = entry.uri.path.substring(includeDirectory.path.length);
      headers[name] = sha256.convert(entry.readAsBytesSync()).toString();
    }
  }
  return headers;
}

/// Resolves the shared library within an install prefix for [code]'s target.
///
/// The library directory is `lib` on most platforms, `lib64` where the
/// toolchain's `CMAKE_INSTALL_LIBDIR` says so, and `bin` on Windows.
File _libraryFile(Uri prefix, CodeConfig code) {
  final file = _libraryFileOrNull(prefix, code.targetOS);
  if (file == null) {
    throw FileSystemException(
      'Install prefix holds no MapLibre Native C library. Rebuild it with '
      '`mise run build`, or delete $installPrefixPointer to stop pointing at '
      'it',
      prefix.toFilePath(),
    );
  }
  return file;
}

/// Resolves the shared library within an install prefix, or null when the
/// prefix holds none.
File? _libraryFileOrNull(Uri prefix, OS targetOS) {
  final fileName = switch (targetOS) {
    OS.macOS || OS.iOS => 'lib$_libraryName.dylib',
    OS.windows => '$_libraryName.dll',
    _ => 'lib$_libraryName.so',
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
  return null;
}
