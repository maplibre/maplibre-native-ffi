// Supplies the MapLibre Native C library as a code asset, which is what the
// `@Native` declarations in `lib/src/internal/c/maplibre_native_c.g.dart`
// resolve against at run time.

import 'dart:convert';
import 'dart:io';

import 'package:code_assets/code_assets.dart';
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

void main(List<String> arguments) async {
  await build(arguments, (input, output) async {
    if (!input.config.buildCodeAssets) {
      return;
    }

    final pointer = File.fromUri(
      input.packageRoot.resolve(installPrefixPointer),
    );
    // Registered before the existence check so that writing the file later
    // reruns this hook.
    output.dependencies.add(pointer.absolute.uri);
    if (!pointer.existsSync()) {
      // Emit no asset rather than failing. Tooling that never calls native
      // code runs fine without one, and `dart run tool/ffigen.dart` is exactly
      // that: it generates the declarations that would resolve this asset.
      // Anything that does call native code fails to resolve the asset id.
      return;
    }

    // A pointer that names a prefix without a library, or one built for
    // another target, is a broken configuration rather than an absent one, and
    // this fails rather than quietly looking elsewhere. `mise run clean` leaves
    // the pointer behind, so the messages say how to get out of it.
    final prefix = Uri.directory(pointer.readAsStringSync().trim());
    _verifyPlatform(prefix, input.config.code);
    final library = _libraryFile(prefix, input.config.code.targetOS);
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

/// Rejects an install prefix built for a target other than this build's.
///
/// The pointer names whichever preset was built last, and a library file name
/// alone does not distinguish `android-arm64` from `android-x64`, or either
/// from a host `linux-x64` build. The descriptor records what the artifact was
/// built for, so it is what decides.
void _verifyPlatform(Uri prefix, CodeConfig code) {
  final descriptor = File.fromUri(
    prefix.resolve('share/$_libraryName/artifact.json'),
  );
  if (!descriptor.existsSync()) {
    throw FileSystemException(
      'Install prefix holds no $_libraryName descriptor. Rebuild it with '
      '`mise run build`, or delete $installPrefixPointer to stop pointing at '
      'it',
      descriptor.path,
    );
  }

  final expected = _targetPlatform(code);
  final actual =
      (jsonDecode(descriptor.readAsStringSync())
          as Map<String, Object?>)['targetPlatform'];
  if (actual != expected) {
    throw StateError(
      'Install prefix was built for $actual, but this build targets '
      '$expected. Build that preset and point $installPrefixPointer at it.',
    );
  }
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

/// Resolves the shared library within an install prefix for [targetOS].
///
/// The library directory is `lib` on most platforms, `lib64` where the
/// toolchain's `CMAKE_INSTALL_LIBDIR` says so, and `bin` on Windows.
File _libraryFile(Uri prefix, OS targetOS) {
  final file = _libraryFileOrNull(prefix, targetOS);
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
  return null;
}
