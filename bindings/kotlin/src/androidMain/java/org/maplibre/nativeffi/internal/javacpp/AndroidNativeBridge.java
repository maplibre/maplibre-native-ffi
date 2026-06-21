package org.maplibre.nativeffi.internal.javacpp;

import org.bytedeco.javacpp.annotation.Cast;
import org.bytedeco.javacpp.annotation.Name;
import org.bytedeco.javacpp.annotation.Platform;
import org.bytedeco.javacpp.annotation.Properties;
import org.bytedeco.javacpp.annotation.Raw;

/** Android-only JavaCPP bridge helpers that need direct JNI call context. */
@Properties(
    inherit = MaplibreNativeCConfig.class,
    value = @Platform(cinclude = "android_native_bridge.h"))
public final class AndroidNativeBridge {
  private AndroidNativeBridge() {}

  @Name("mln_android_init_javacpp")
  public static native @Cast("mln_status") int initialize(@Raw(withEnv = true) Object context);
}
