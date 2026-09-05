package org.maplibre.nativeffi.internal.javacpp;

import org.bytedeco.javacpp.Pointer;
import org.bytedeco.javacpp.annotation.Cast;
import org.bytedeco.javacpp.annotation.Name;
import org.bytedeco.javacpp.annotation.Platform;
import org.bytedeco.javacpp.annotation.Properties;
import org.bytedeco.javacpp.annotation.Raw;

/** Android-only JavaCPP helpers for JNI context and borrowed arrays. */
@Properties(
    inherit = MaplibreNativeCConfig.class,
    value = @Platform(include = "android_image_bridge.h"))
public final class AndroidNativeBridge {
  private AndroidNativeBridge() {}

  @Name("mln_android_init")
  public static native @Cast("mln_status") int initialize(@Raw(withEnv = true) Object context);

  @Name("mln_android_set_style_image")
  public static native @Cast("mln_status") int setStyleImage(
      @Cast("mln_map") long map,
      @Cast("const mln_buffer_view*") Pointer imageId,
      @Cast("const mln_premultiplied_rgba8_image*") Pointer image,
      @Cast("const uint8_t*") byte[] pixels,
      @Cast("const mln_style_image_options*") Pointer options);

  @Name("mln_android_add_image_source_image")
  public static native @Cast("mln_status") int addImageSourceImage(
      @Cast("mln_map") long map,
      @Cast("const mln_buffer_view*") Pointer sourceId,
      @Cast("const mln_lat_lng*") Pointer coordinates,
      @Cast("size_t") long coordinateCount,
      @Cast("const mln_premultiplied_rgba8_image*") Pointer image,
      @Cast("const uint8_t*") byte[] pixels);

  @Name("mln_android_set_image_source_image")
  public static native @Cast("mln_status") int setImageSourceImage(
      @Cast("mln_map") long map,
      @Cast("const mln_buffer_view*") Pointer sourceId,
      @Cast("const mln_premultiplied_rgba8_image*") Pointer image,
      @Cast("const uint8_t*") byte[] pixels);
}
