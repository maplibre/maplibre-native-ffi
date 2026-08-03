// Generates the private raw declarations for the MapLibre Native C API.
//
// Run with `mise run //bindings/dart:ffigen` from the repository root.
//
// Pass `--output <path>` to write the bindings somewhere other than the
// committed file.

import 'dart:io';

import 'package:ffigen/ffigen.dart';
import 'package:maplibre_native_ffi/src/internal/c/native_asset.dart';

void main(List<String> args) {
  final packageRoot = Platform.script.resolve('../');
  final repoRoot = packageRoot.resolve('../../');

  final headerRoot = repoRoot.resolve('include/');
  final publicHeader = headerRoot.resolve('maplibre_native_c.h');
  final publicHeaderDir = headerRoot.resolve('maplibre_native_c/');
  // The callback adapter is a public header outside the umbrella, so it needs
  // its own entry point.
  final adapterHeader = publicHeaderDir.resolve('callback_adapter.h');

  FfiGenerator(
    output: Output(
      dartFile: _outputFile(args, packageRoot),
      commentType: const CommentType.none(),
      // The native library arrives as a code asset from `hook/build.dart`,
      // which the Dart runtime resolves by asset id. That is only reachable
      // through `@Native`, so the declarations are static rather than a wrapper
      // class over a DynamicLibrary.
      style: const NativeExternalBindings(assetId: nativeAssetId),
      preamble: '''
// ignore_for_file: always_specify_types
// ignore_for_file: camel_case_types
// ignore_for_file: non_constant_identifier_names
// ignore_for_file: unused_element
// ignore_for_file: unused_field
''',
    ),
    headers: Headers(
      entryPoints: [publicHeader, adapterHeader],
      // Keep generation to the repository's own headers so that transitively
      // included system and Vulkan declarations stay out of the bindings.
      include: (header) =>
          header == publicHeader ||
          header.toString().startsWith(publicHeaderDir.toString()),
      compilerOptions: [
        '-I${headerRoot.toFilePath()}',
        '-I${repoRoot.resolve('third_party/maplibre-native/vendor/Vulkan-Headers/include/').toFilePath()}',
        // libclang does not ship its own builtin headers, so point it at the
        // resource directory of the clang on PATH.
        '-isystem',
        '${_clangResourceDir()}/include',
      ],
    ),
    enums: Enums(
      include: Declarations.includeAll,
      silenceWarning: true,
      // Status is an integer in the C API, and every binding's raw layer
      // mirrors it as one: `i32` in Rust, `c.mln_status` in Zig. A Dart enum
      // here would also force ffigen to wrap each status-returning function in
      // a converting call, which hides the `@Native` declaration behind a
      // private name and puts the function's address out of reach.
      style: (declaration, suggested) =>
          declaration.originalName == 'mln_status'
          ? EnumStyle.intConstants
          : (suggested ?? EnumStyle.dartEnum),
    ),
    functions: Functions.includeAll,
    globals: Globals.includeAll,
    macros: Macros.includeAll,
    structs: Structs.includeAll,
    typedefs: Typedefs.includeAll,
    unions: Unions.includeAll,
    unnamedEnums: UnnamedEnums.includeAll,
  ).generate();
}

Uri _outputFile(List<String> args, Uri packageRoot) {
  final flag = args.indexOf('--output');
  if (flag >= 0) {
    if (flag + 1 >= args.length) {
      throw ArgumentError('--output requires a path argument.');
    }
    return packageRoot.resolve(args[flag + 1]);
  }
  return packageRoot.resolve('lib/src/internal/c/maplibre_native_c.g.dart');
}

String _clangResourceDir() {
  final result = Process.runSync(_clangExecutable(), ['-print-resource-dir']);
  if (result.exitCode != 0) {
    throw StateError('clang -print-resource-dir failed: ${result.stderr}');
  }
  return (result.stdout as String).trim();
}

String _clangExecutable() {
  final executableName = Platform.isWindows ? 'clang.exe' : 'clang';
  final separator = Platform.isWindows ? ';' : ':';
  for (final directory in (Platform.environment['PATH'] ?? '').split(
    separator,
  )) {
    if (directory.isEmpty) {
      continue;
    }
    final candidate = File(
      '$directory${Platform.pathSeparator}$executableName',
    );
    if (!candidate.existsSync()) {
      continue;
    }
    final normalized = candidate.absolute.path.replaceAll('\\', '/');
    if (normalized.contains('/mise/shims/')) {
      continue;
    }
    // The Emscripten SDK ships a clang whose builtin headers expect its own
    // sysroot, so parsing host headers with it fails on <stdint.h>. The
    // repository's bootstrap installs that SDK, so it is often on PATH.
    if (normalized.contains('/emsdk/')) {
      continue;
    }
    // Resource-directory discovery needs the compiler itself: an inactive
    // mise shim or ccache wrapper resolves another command through PATH.
    final resolved = candidate.resolveSymbolicLinksSync().replaceAll('\\', '/');
    final resolvedName = resolved.substring(resolved.lastIndexOf('/') + 1);
    if (!resolvedName.toLowerCase().startsWith('clang')) {
      continue;
    }
    return candidate.path;
  }
  throw StateError('a direct clang executable is required on PATH');
}
