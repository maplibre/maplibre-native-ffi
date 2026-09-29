package org.maplibre.nativeffi.internal.javacpp;

import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.annotation.Cast;
import org.bytedeco.javacpp.annotation.Name;
import org.bytedeco.javacpp.annotation.Platform;
import org.bytedeco.javacpp.annotation.Properties;
import org.bytedeco.javacpp.annotation.Raw;

/** Android-only JavaCPP helpers for JNI context and the plugin registration entry point. */
@Properties(inherit = MaplibreNativeCConfig.class, value = @Platform(include = "plugin_bridge.h"))
public final class AndroidNativeBridge {
  private AndroidNativeBridge() {}

  @Name("mln_android_plugin_register_function_v1")
  public static native @Cast("uintptr_t") long pluginRegisterFunctionV1();

  @Name("mln_android_init")
  public static native @Cast("mln_status") int initialize(
      @Raw(withEnv = true) Object context, @Cast("mln_diagnostic*") Pointer diagnostic);
}
