import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

void main() {
  /// Resolves and loads the code asset `hook/build.dart` declares. This runs in
  /// CI where the rest of the suite does not, so it is what proves a downloaded
  /// artifact works. It goes through the public API so the ABI-version gate
  /// runs too. Every artifact compiles exactly one renderer, so the mask is
  /// never empty.
  test('loads the native library', () {
    expect(Maplibre.supportedRenderBackends().bits, isNonZero);
  });
}
