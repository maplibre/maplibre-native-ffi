package org.maplibre.nativejni.internal.javacpp;

import java.nio.charset.StandardCharsets;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;

/** Small helpers for adapting the curated Java API to JavaCPP's generated C layer. */
public final class JavaCppSupport {
  private JavaCppSupport() {}

  public static String cString(BytePointer pointer) {
    return pointer == null || pointer.isNull() ? "" : pointer.getString(StandardCharsets.UTF_8);
  }

  public static BytePointer utf8(String value) {
    return value == null ? null : new BytePointer(value, StandardCharsets.UTF_8);
  }

  public static Pointer pointer(long address) {
    return new AddressPointer(address);
  }

  public static Pointer pointerOrNull(long address) {
    return address == 0 ? null : pointer(address);
  }

  public static MaplibreNativeC.mln_runtime runtime(long address) {
    return new MaplibreNativeC.mln_runtime(pointer(address));
  }

  public static MaplibreNativeC.mln_map map(long address) {
    return new MaplibreNativeC.mln_map(pointer(address));
  }

  public static MaplibreNativeC.mln_map_projection projection(long address) {
    return new MaplibreNativeC.mln_map_projection(pointer(address));
  }

  public static MaplibreNativeC.mln_resource_request_handle resourceRequestHandle(long address) {
    return new MaplibreNativeC.mln_resource_request_handle(pointer(address));
  }

  public static MaplibreNativeC.mln_render_session renderSession(long address) {
    return new MaplibreNativeC.mln_render_session(pointer(address));
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  public static long outAddress(PointerPointer<?> out, Class<? extends Pointer> type) {
    var pointer = ((PointerPointer) out).get((Class) type, 0);
    return pointer == null || pointer.isNull() ? 0 : pointer.address();
  }

  private static final class AddressPointer extends Pointer {
    AddressPointer(long address) {
      super((Pointer) null);
      this.address = address;
    }
  }
}
