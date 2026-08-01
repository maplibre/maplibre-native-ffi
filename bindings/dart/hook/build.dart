// Supplies the MapLibre Native C library as a code asset, which is what the
// `@Native` declarations in `lib/src/internal/c/maplibre_native_c.g.dart`
// resolve against at run time.
//
// The library comes from a local build when one is configured, and otherwise
// from the published snapshot release. That contract is specified in
// docs/src/content/docs/development/binding-specification.md under "Native
// Artifact Acquisition"; the Rust build script implements the same one.

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
/// Build hooks run in a semi-hermetic environment that strips arbitrary
/// environment variables, so the prefix arrives in a file rather than in
/// `MLN_FFI_NATIVE_LIBRARY` the way it does for the other bindings.
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
    // them through it: `macos-arm64-egl` ships ANGLE's libEGL and libGLESv2,
    // resolved relative to the loader. Bundling only the C API library would
    // leave those behind, so declare them too and let the SDK place them
    // together.
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
/// alone does not distinguish `android-arm64` from `android-x64`, or either
/// from a host `linux-x64` build. The descriptor records what the artifact was
/// built for, so it is what decides.
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

  // Only an explicit request conflicts. Absent one, the prefix's own renderer
  // is the answer: a local build is a deliberate choice of artifact, so the
  // platform default the download path applies has nothing to say about it.
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
    OS.linux => 'linux-$architecture',
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
    // A pointer that names a prefix without a library, or one built for
    // another target, is a broken configuration rather than an absent one.
    // Downloading instead would turn an explicit opt-out of the network into a
    // silent opt-in, so this fails and says how to get out of it.
    final prefix = Uri.directory(pointer.readAsStringSync().trim());
    _verifyLocalPrefix(prefix, input);
    _libraryFile(prefix, input.config.code);
    return prefix;
  }

  final preset = _resolvePreset(input);
  final prefix = await _downloadPrefix(input, preset);
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

/// The presets `.github/workflows/snapshots.yml` publishes a shared library
/// for, keyed by the target they serve. The OpenHarmony presets build from
/// source and ship no archive, so they are absent here; so is device iOS, whose
/// `ios-arm64-metal` archive holds only a static library.
const Map<String, ({String defaultBackend, Map<String, String> backends})>
_platformTargets = {
  'linux-x64': (defaultBackend: 'vulkan', backends: _openglEgl),
  'linux-arm64': (defaultBackend: 'vulkan', backends: _openglEgl),
  'macos-arm64': (defaultBackend: 'metal', backends: _appleDesktop),
  'windows-x64': (defaultBackend: 'vulkan', backends: _openglWgl),
  'windows-arm64': (defaultBackend: 'vulkan', backends: _openglWgl),
  'android-arm64': (defaultBackend: 'opengl', backends: _openglEgl),
  'android-x64': (defaultBackend: 'opengl', backends: _openglEgl),
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
/// The snapshot release is floating: its asset URLs never change, so a cache
/// keyed on the URL would serve stale bytes forever. `SHA256SUMS` changing is
/// the exact signal that the artifacts moved, so its digest is the cache key.
///
/// A publish replaces `SHA256SUMS` and the archives as separate assets, so a
/// build that starts mid-publish can pair one generation's checksum with the
/// other's bytes. That reads as a mismatch without either file being corrupt,
/// so a mismatch is retried once against a freshly fetched checksum file. A
/// mismatch that survives an unchanged checksum stays fatal, which is what
/// keeps a genuinely corrupt download out of the cache.
///
/// The retry gives up every cached answer with it — the offline fallback and
/// the cache hit alike. Once bytes have failed verification the release is no
/// longer trustworthy for this build, and answering that from disk, whether on
/// a later timeout or on a digest already present, would hide an unverified
/// download behind an older artifact.
Future<Uri> _downloadPrefix(
  BuildInput input,
  _Preset preset, {
  bool afterMismatch = false,
}) async {
  final cacheDirectory = Directory.fromUri(
    input.outputDirectoryShared.resolve('${preset.name}/'),
  )..createSync(recursive: true);

  final String checksums;
  try {
    checksums = utf8.decode(
      await _fetch('$_releaseBaseUrl/$_snapshotTag/SHA256SUMS'),
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
  // A retry after a mismatch takes no cache hit either. A checksum file that
  // crossed back to an older generation resolves to a digest already on disk,
  // which would answer the failed verification from the cache without proving
  // any fresh bytes.
  if (prefix.existsSync() && !afterMismatch) {
    return prefix.uri;
  }

  final archiveName = '$_libraryName-${preset.name}.tar.gz';
  final expected = _checksumFor(checksums, archiveName);
  stderr.writeln('Downloading $archiveName from the $_snapshotTag release.');
  final Uint8List archive;
  try {
    archive = await _fetch('$_releaseBaseUrl/$_snapshotTag/$archiveName');
  } on Object catch (error) {
    // A publish race can list an archive in SHA256SUMS before uploading it.
    // Only the fetch falls back; the checksum check below stays fatal, so a
    // corrupt download is never papered over with a stale artifact.
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
      return _downloadPrefix(input, preset, afterMismatch: true);
    }
    throw StateError(
      'Checksum mismatch for $archiveName: expected $expected, got $actual.',
    );
  }

  _extract(archive, cacheDirectory, prefix, preset, archiveName);
  return prefix.uri;
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
      // root can still land outside the scratch tree. Rust's tar reader rejects
      // traversal for us; this is the equivalent guard. It compares resolved
      // filesystem paths rather than URI paths, so a separator that only the
      // platform treats as one — a backslash on Windows — cannot slip between
      // the two spellings, and it requires that separator so that a sibling
      // sharing the scratch directory's name as a prefix is not accepted.
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

/// Bounds every request, so a host that accepts the connection and then stops
/// answering fails into the cache fallback rather than hanging the build. The
/// whole-request bound covers the body too, so it allows for a slow link
/// pulling a 30 MB archive.
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
/// A git dependency pinned at one commit gets whatever the floating release
/// currently holds, which is built from another. Comparing commits would warn
/// constantly, because the publish is gated on input digests and the artifact's
/// commit lags by design. Differing headers are the condition that matters.
void _warnOnHeaderSkew(
  BuildInput input,
  BuildOutputBuilder output,
  Uri prefix,
  _Preset preset,
) {
  // The package sits at bindings/dart, and both git and path dependencies carry
  // the whole checkout.
  final checkoutInclude = input.packageRoot.resolve('../../include/');
  final checkout = _publicHeaders(checkoutInclude);
  final artifact = _publicHeaders(prefix.resolve('include/'));
  if (checkout == null || artifact == null) {
    return;
  }

  // Editing a public header in a path dependency has to rerun this hook, or
  // the warning it produces goes stale against the declarations it describes.
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
  // Device iOS is the one target this project builds no shared library for:
  // `cmake/platform/apple.cmake` clears MLN_FFI_SHARED_SUPPORTED there, so the
  // prefix holds `libmaplibre-native-c.a` alone. A code asset carries a
  // library the Dart runtime loads, which a static archive cannot be, so this
  // says so rather than reporting the prefix as empty. The download path
  // reaches the same conclusion earlier, by leaving device iOS out of the
  // published-preset table.
  if (code.targetOS == OS.iOS && code.iOS.targetSdk != IOSSdk.iPhoneSimulator) {
    throw UnsupportedError(
      'MapLibre Native builds a static archive for device iOS, which cannot '
      'become a Dart code asset. The iOS simulator is supported.',
    );
  }

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
