package org.maplibre.nativejni.internal.bridge;

import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the BaseNative C API coverage group. */
public final class BaseNative {
  private static final ThreadLocal<String> JAVA_DIAGNOSTIC = new ThreadLocal<>();

  private BaseNative() {}

  public static void setThreadDiagnostic(String diagnostic) {
    JAVA_DIAGNOSTIC.set(diagnostic);
  }

  public static long mln_c_version() {
    return MaplibreNativeC.mln_c_version();
  }

  public static int mln_supported_render_backend_mask() {
    return MaplibreNativeC.mln_supported_render_backend_mask();
  }

  public static String mln_thread_last_error_message() {
    var diagnostic = JAVA_DIAGNOSTIC.get();
    if (diagnostic != null) {
      JAVA_DIAGNOSTIC.remove();
      return diagnostic;
    }
    return JavaCppSupport.cString(MaplibreNativeC.mln_thread_last_error_message());
  }
}
