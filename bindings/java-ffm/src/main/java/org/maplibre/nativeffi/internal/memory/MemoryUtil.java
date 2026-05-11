package org.maplibre.nativeffi.internal.memory;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.nio.charset.StandardCharsets;
import java.util.Objects;

/** Internal FFM memory helpers. */
public final class MemoryUtil {
  private MemoryUtil() {}

  public static MemorySegment allocateCString(Arena arena, String value) {
    Objects.requireNonNull(arena, "arena");
    Objects.requireNonNull(value, "value");
    rejectEmbeddedNul(value);
    return arena.allocateFrom(value, StandardCharsets.UTF_8);
  }

  public static MemorySegment allocateOptionalCString(Arena arena, String value) {
    return value == null ? MemorySegment.NULL : allocateCString(arena, value);
  }

  public static void rejectEmbeddedNul(String value) {
    Objects.requireNonNull(value, "value");
    if (value.indexOf('\0') >= 0) {
      throw new IllegalArgumentException(
          "C string inputs must not contain embedded NUL characters");
    }
  }

  public static String copyCString(MemorySegment address) {
    if (isNull(address)) {
      return "";
    }
    return address.reinterpret(Long.MAX_VALUE).getString(0, StandardCharsets.UTF_8);
  }

  public static String copyStringView(MemorySegment address, long byteCount) {
    if (byteCount == 0 || isNull(address)) {
      return "";
    }
    var bytes = address.reinterpret(byteCount).toArray(ValueLayout.JAVA_BYTE);
    return new String(bytes, StandardCharsets.UTF_8);
  }

  public static byte[] copyBytes(MemorySegment address, long byteCount) {
    if (byteCount == 0 || isNull(address)) {
      return new byte[0];
    }
    return address.reinterpret(byteCount).toArray(ValueLayout.JAVA_BYTE);
  }

  public static MemorySegment allocatePointer(Arena arena) {
    var pointer = arena.allocate(ValueLayout.ADDRESS);
    pointer.set(ValueLayout.ADDRESS, 0, MemorySegment.NULL);
    return pointer;
  }

  public static boolean isNull(MemorySegment address) {
    return address == null || address.address() == 0;
  }
}
