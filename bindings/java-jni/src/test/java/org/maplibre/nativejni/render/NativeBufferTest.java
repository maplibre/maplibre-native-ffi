package org.maplibre.nativejni.render;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import org.junit.jupiter.api.Test;

class NativeBufferTest {
  @Test
  void rejectsCapacitiesBeyondDirectByteBufferLimit() {
    assertThrows(
        IllegalArgumentException.class, () -> NativeBuffer.allocate((long) Integer.MAX_VALUE + 1));
  }

  @Test
  void closeInvalidatesWrapper() {
    var buffer = NativeBuffer.allocate(4);
    assertEquals(4, buffer.byteLength());

    buffer.close();

    assertThrows(IllegalStateException.class, buffer::byteLength);
    assertThrows(IllegalStateException.class, buffer::toByteArray);
  }
}
