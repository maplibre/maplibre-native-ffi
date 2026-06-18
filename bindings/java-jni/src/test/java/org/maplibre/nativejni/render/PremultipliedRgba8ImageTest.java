package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;

import org.junit.jupiter.api.Test;

class PremultipliedRgba8ImageTest {
  // BND-069: image descriptors snapshot caller-owned pixel arrays and return copies.

  @Test
  void bnd069PixelsSnapshotAndReturnCopies() {
    var source = new byte[] {1, 2, 3, 4};
    var image = new PremultipliedRgba8Image(1, 1, 4, source);
    source[0] = 9;

    var first = image.pixels();
    assertArrayEquals(new byte[] {1, 2, 3, 4}, first);
    first[0] = 8;
    assertArrayEquals(new byte[] {1, 2, 3, 4}, image.pixels());
  }
}
