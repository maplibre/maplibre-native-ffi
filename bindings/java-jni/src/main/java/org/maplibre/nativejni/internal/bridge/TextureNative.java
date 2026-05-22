package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the TextureNative C API coverage group. */
public final class TextureNative {
  private TextureNative() {}

  public static native int mln_metal_owned_texture_descriptor_default();

  public static native int mln_metal_borrowed_texture_descriptor_default();

  public static native int mln_vulkan_owned_texture_descriptor_default();

  public static native int mln_vulkan_borrowed_texture_descriptor_default();

  public static native int mln_texture_image_info_default();

  public static native int mln_metal_owned_texture_attach(
      long map, int width, int height, double scaleFactor, long device, long[] outSession);

  public static native int mln_metal_borrowed_texture_attach(
      long map, int width, int height, double scaleFactor, long texture, long[] outSession);

  public static native int mln_vulkan_owned_texture_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex,
      long[] outSession);

  public static native int mln_vulkan_borrowed_texture_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex,
      long image,
      long imageView,
      int format,
      int initialLayout,
      Integer finalLayout,
      long[] outSession);

  public static native int mln_texture_read_premultiplied_rgba8(
      long session, byte[] outData, int[] outInfo, long[] outByteLength);

  public static native int mln_metal_owned_texture_acquire_frame(
      long session, long[] outLongs, int[] outInts, double[] outDoubles);

  public static native int mln_metal_owned_texture_release_frame(
      long session, long[] longs, int[] ints, double[] doubles);

  public static native int mln_vulkan_owned_texture_acquire_frame(
      long session, long[] outLongs, int[] outInts, double[] outDoubles);

  public static native int mln_vulkan_owned_texture_release_frame(
      long session, long[] longs, int[] ints, double[] doubles);
}
