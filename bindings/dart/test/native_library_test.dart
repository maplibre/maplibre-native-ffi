import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  /// Calls into the native library so that the code asset `hook/build.dart`
  /// declares is actually resolved and loaded. The rest of the suite covers
  /// this incidentally, but it holds native handles across isolate boundaries
  /// and so stays out of CI until #412 is fixed; this file does neither, which
  /// makes it the one that can prove a downloaded artifact works.
  ///
  /// Through the public API rather than the generated declarations, so the
  /// ABI-version gate runs too: an artifact this binding cannot talk to should
  /// fail here rather than pass. Every artifact compiles exactly one renderer,
  /// so the mask is never empty.
  test('loads the native library', () {
    expect(Maplibre.supportedRenderBackends().bits, isNonZero);
  });
}
