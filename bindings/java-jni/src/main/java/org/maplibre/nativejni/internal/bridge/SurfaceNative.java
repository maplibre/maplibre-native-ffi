package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the SurfaceNative C API coverage group. */
public final class SurfaceNative {
  private SurfaceNative() {}

  public static native int mln_metal_surface_descriptor_default();

  public static native int mln_vulkan_surface_descriptor_default();

  public static native int mln_metal_surface_attach();

  public static native int mln_vulkan_surface_attach();
}
