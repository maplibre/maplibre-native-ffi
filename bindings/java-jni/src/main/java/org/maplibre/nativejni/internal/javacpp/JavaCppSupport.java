package org.maplibre.nativejni.internal.javacpp;

import java.nio.charset.StandardCharsets;
import java.util.Optional;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.PointerPointer;

/** Small helpers for adapting the curated Java API to JavaCPP's generated C layer. */
public final class JavaCppSupport {
  private static final ThreadLocal<String> JAVA_DIAGNOSTIC = new ThreadLocal<>();

  private JavaCppSupport() {}

  public static void setThreadDiagnostic(String diagnostic) {
    JAVA_DIAGNOSTIC.set(diagnostic);
  }

  public static Optional<String> takeThreadDiagnostic() {
    var diagnostic = JAVA_DIAGNOSTIC.get();
    if (diagnostic != null) {
      JAVA_DIAGNOSTIC.remove();
    }
    return Optional.ofNullable(diagnostic);
  }

  public static String cString(BytePointer pointer) {
    return pointer == null || pointer.isNull() ? "" : pointer.getString(StandardCharsets.UTF_8);
  }

  public static String utf8String(BytePointer pointer, long byteLength) {
    if (pointer == null || pointer.isNull() || byteLength == 0) {
      return "";
    }
    var bytes = new byte[Math.toIntExact(byteLength)];
    pointer.get(bytes, 0, bytes.length);
    return new String(bytes, StandardCharsets.UTF_8);
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

  public static <T extends Pointer> PointerPointer<T> outPointer(Class<T> type) {
    var out = new PointerPointer<T>(1);
    out.put(0, (Pointer) null);
    return out;
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
