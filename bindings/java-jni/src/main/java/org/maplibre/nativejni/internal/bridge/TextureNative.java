package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the TextureNative C API coverage group. */
public final class TextureNative {
  private TextureNative() {}

  public static native int mln_metal_owned_texture_descriptor_default();

  public static native int mln_metal_borrowed_texture_descriptor_default();

  public static native int mln_vulkan_owned_texture_descriptor_default();

  public static native int mln_vulkan_borrowed_texture_descriptor_default();

  public static native int mln_texture_image_info_default();

  public static native int mln_metal_owned_texture_attach();

  public static native int mln_metal_borrowed_texture_attach();

  public static native int mln_vulkan_owned_texture_attach();

  public static native int mln_vulkan_borrowed_texture_attach();

  public static native int mln_texture_read_premultiplied_rgba8();

  public static native int mln_metal_owned_texture_acquire_frame();

  public static native int mln_metal_owned_texture_release_frame();

  public static native int mln_vulkan_owned_texture_acquire_frame();

  public static native int mln_vulkan_owned_texture_release_frame();
}
