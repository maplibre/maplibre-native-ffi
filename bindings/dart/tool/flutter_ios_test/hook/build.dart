import 'dart:io';

import 'package:code_assets/code_assets.dart';
import 'package:hooks/hooks.dart';

// Only the test host bundles tests/graphics; the published binding leaves it out.
Future<void> main(List<String> arguments) async {
  await build(arguments, (input, output) async {
    if (!input.config.buildCodeAssets) return;
    final library = File.fromUri(
      input.packageRoot.resolve('../native/lib/libmln_test_graphics.dylib'),
    );
    output.dependencies.add(library.uri);
    final bundled = await library.copy(
      input.outputDirectory.resolve('libmln_test_graphics.dylib').toFilePath(),
    );
    output.assets.code.add(
      CodeAsset(
        package: input.packageName,
        name: 'test_graphics',
        linkMode: DynamicLoadingBundled(),
        file: bundled.uri,
      ),
    );
  });
}
