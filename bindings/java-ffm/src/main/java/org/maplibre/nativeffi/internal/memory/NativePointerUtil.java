package org.maplibre.nativeffi.internal.memory;

import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.render.NativePointer;

/** Internal conversion between public opaque pointer values and FFM segments. */
public final class NativePointerUtil {
  private NativePointerUtil() {}

  public static MemorySegment toSegment(NativePointer pointer) {
    if (pointer == null || pointer.isNull()) {
      return MemorySegment.NULL;
    }
    return MemorySegment.ofAddress(pointer.address());
  }

  public static NativePointer fromSegment(MemorySegment segment) {
    if (MemoryUtil.isNull(segment)) {
      return NativePointer.NULL;
    }
    return NativePointer.ofAddress(segment.address());
  }
}
