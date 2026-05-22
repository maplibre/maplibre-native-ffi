package org.maplibre.nativejni.internal.bridge;

/** JNI declarations for the SurfaceNative C API coverage group. */
public final class SurfaceNative {
  private SurfaceNative() {}

  public static native int mln_metal_surface_descriptor_default();

  public static native int mln_vulkan_surface_descriptor_default();

  public static native int mln_metal_surface_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long device,
      long layer,
      long[] outSession);

  public static native int mln_vulkan_surface_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex,
      long surface,
      long[] outSession);
}
