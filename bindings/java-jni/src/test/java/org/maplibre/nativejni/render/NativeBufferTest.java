package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.Test;

// Support invariant for BND-166: NativeBuffer is Java JNI's reusable CPU readback buffer,
// so its capacity and closed-state checks protect public readback ownership behavior.
class NativeBufferTest {
  @Test
  void bnd166RejectsCapacitiesBeyondDirectByteBufferLimit() {
    assertThrows(
        IllegalArgumentException.class, () -> NativeBuffer.allocate((long) Integer.MAX_VALUE + 1));
  }

  @Test
  void bnd166CloseInvalidatesWrapper() {
    var buffer = NativeBuffer.allocate(4);
    assertEquals(4, buffer.byteLength());

    buffer.close();

    assertThrows(IllegalStateException.class, buffer::byteLength);
    assertThrows(IllegalStateException.class, buffer::toByteArray);
  }
}
