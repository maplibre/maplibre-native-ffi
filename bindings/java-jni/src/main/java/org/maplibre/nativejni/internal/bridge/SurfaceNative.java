package org.maplibre.nativejni.internal.bridge;

import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the SurfaceNative C API coverage group. */
public final class SurfaceNative {
  private SurfaceNative() {}

  public static int mln_metal_surface_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long device,
      long layer,
      long[] outSession) {
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_metal_surface_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    descriptor.context().device(JavaCppSupport.pointerOrNull(device));
    descriptor.layer(JavaCppSupport.pointerOrNull(layer));
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status = MaplibreNativeC.mln_metal_surface_attach(JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  public static int mln_vulkan_surface_attach(
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
      long[] outSession) {
    if (graphicsQueueFamilyIndex < 0) {
      BaseNative.setThreadDiagnostic("graphics queue family index must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_vulkan_surface_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    setVulkanContext(
        descriptor.context(),
        instance,
        physicalDevice,
        device,
        graphicsQueue,
        graphicsQueueFamilyIndex);
    descriptor.surface(JavaCppSupport.pointerOrNull(surface));
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status =
        MaplibreNativeC.mln_vulkan_surface_attach(JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  private static int validateExtent(int width, int height, double scaleFactor) {
    if (width < 0 || height < 0) {
      BaseNative.setThreadDiagnostic("width and height must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!Double.isFinite(scaleFactor) || scaleFactor <= 0) {
      BaseNative.setThreadDiagnostic("scale factor must be positive and finite");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    return MaplibreNativeC.MLN_STATUS_OK;
  }

  private static void setExtent(
      MaplibreNativeC.mln_render_target_extent extent, int width, int height, double scaleFactor) {
    extent.width(width);
    extent.height(height);
    extent.scale_factor(scaleFactor);
  }

  private static void setVulkanContext(
      MaplibreNativeC.mln_vulkan_context_descriptor context,
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex) {
    context.instance(JavaCppSupport.pointerOrNull(instance));
    context.physical_device(JavaCppSupport.pointerOrNull(physicalDevice));
    context.device(JavaCppSupport.pointerOrNull(device));
    context.graphics_queue(JavaCppSupport.pointerOrNull(graphicsQueue));
    context.graphics_queue_family_index(graphicsQueueFamilyIndex);
  }
}
