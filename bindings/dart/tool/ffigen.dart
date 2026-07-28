// Generates the private raw declarations for the MapLibre Native C API.
//
// Run with `mise run //bindings/dart:ffigen` from the repository root.
//
// Pass `--output <path>` to write the bindings elsewhere; `ffigen-check` uses
// this to regenerate into a scratch file and compare it against the committed
// one.

import 'dart:io';

import 'package:ffigen/ffigen.dart';

void main(List<String> args) {
  final packageRoot = Platform.script.resolve('../');
  final repoRoot = packageRoot.resolve('../../');

  final headerRoot = repoRoot.resolve('include/');
  final publicHeader = headerRoot.resolve('maplibre_native_c.h');
  final publicHeaderDir = headerRoot.resolve('maplibre_native_c/');
  final dartShimHeader = repoRoot.resolve('src/c_api/dart_shim.h');

  FfiGenerator(
    output: Output(
      dartFile: _outputFile(args, packageRoot),
      commentType: const CommentType.none(),
      style: const DynamicLibraryBindings(
        wrapperName: 'MaplibreNativeC',
        wrapperDocComment:
            'Private generated declarations for the MapLibre Native C API.',
      ),
      preamble: '''
// ignore_for_file: always_specify_types
// ignore_for_file: camel_case_types
// ignore_for_file: non_constant_identifier_names
// ignore_for_file: unused_element
// ignore_for_file: unused_field
''',
    ),
    headers: Headers(
      entryPoints: [publicHeader, dartShimHeader],
      // Keep generation to the repository's own headers so that transitively
      // included system and Vulkan declarations stay out of the bindings.
      include: (header) =>
          header == publicHeader ||
          header == dartShimHeader ||
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
    enums: const Enums(include: Declarations.includeAll, silenceWarning: true),
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
