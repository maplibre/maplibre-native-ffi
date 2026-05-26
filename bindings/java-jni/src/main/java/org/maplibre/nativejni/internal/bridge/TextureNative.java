package org.maplibre.nativejni.internal.bridge;

import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.PointerPointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;

/** JavaCPP-backed declarations for the TextureNative C API coverage group. */
public final class TextureNative {
  private TextureNative() {}

  public static int mln_metal_owned_texture_attach(
      long map, int width, int height, double scaleFactor, long device, long[] outSession) {
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_metal_owned_texture_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    descriptor.context().device(JavaCppSupport.pointerOrNull(device));
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status =
        MaplibreNativeC.mln_metal_owned_texture_attach(JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  public static int mln_metal_borrowed_texture_attach(
      long map, int width, int height, double scaleFactor, long texture, long[] outSession) {
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_metal_borrowed_texture_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    descriptor.texture(JavaCppSupport.pointerOrNull(texture));
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status =
        MaplibreNativeC.mln_metal_borrowed_texture_attach(JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  public static int mln_vulkan_owned_texture_attach(
      long map,
      int width,
      int height,
      double scaleFactor,
      long instance,
      long physicalDevice,
      long device,
      long graphicsQueue,
      int graphicsQueueFamilyIndex,
      long[] outSession) {
    if (graphicsQueueFamilyIndex < 0) {
      BaseNative.setThreadDiagnostic("graphics queue family index must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_vulkan_owned_texture_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    setVulkanContext(
        descriptor.context(),
        instance,
        physicalDevice,
        device,
        graphicsQueue,
        graphicsQueueFamilyIndex);
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status =
        MaplibreNativeC.mln_vulkan_owned_texture_attach(JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  public static int mln_vulkan_borrowed_texture_attach(
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
      long[] outSession) {
    if (graphicsQueueFamilyIndex < 0
        || format < 0
        || initialLayout < 0
        || (finalLayout != null && finalLayout < 0)) {
      BaseNative.setThreadDiagnostic("Vulkan unsigned fields must be non-negative");
      return MaplibreNativeC.MLN_STATUS_INVALID_ARGUMENT;
    }
    var validation = validateExtent(width, height, scaleFactor);
    if (validation != MaplibreNativeC.MLN_STATUS_OK) {
      return validation;
    }
    var descriptor = MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor_default();
    setExtent(descriptor.extent(), width, height, scaleFactor);
    setVulkanContext(
        descriptor.context(),
        instance,
        physicalDevice,
        device,
        graphicsQueue,
        graphicsQueueFamilyIndex);
    descriptor.image(JavaCppSupport.pointerOrNull(image));
    descriptor.image_view(JavaCppSupport.pointerOrNull(imageView));
    descriptor.format(format);
    descriptor.initial_layout(initialLayout);
    if (finalLayout != null) {
      descriptor.final_layout(finalLayout);
    }
    var out = new PointerPointer<MaplibreNativeC.mln_render_session>(1);
    var status =
        MaplibreNativeC.mln_vulkan_borrowed_texture_attach(
            JavaCppSupport.map(map), descriptor, out);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      outSession[0] = JavaCppSupport.outAddress(out, MaplibreNativeC.mln_render_session.class);
    }
    return status;
  }

  public static int mln_texture_read_premultiplied_rgba8(
      long session, byte[] outData, int[] outInfo, long[] outByteLength) {
    var info = MaplibreNativeC.mln_texture_image_info_default();
    var status =
        outData == null
            ? MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
                JavaCppSupport.renderSession(session), (BytePointer) null, 0, info)
            : MaplibreNativeC.mln_texture_read_premultiplied_rgba8(
                JavaCppSupport.renderSession(session), outData, outData.length, info);
    outInfo[0] = info.width();
    outInfo[1] = info.height();
    outInfo[2] = info.stride();
    outByteLength[0] = info.byte_length();
    return status;
  }

  public static int mln_metal_owned_texture_acquire_frame(
      long session, long[] outLongs, int[] outInts, double[] outDoubles) {
    var frame = new MaplibreNativeC.mln_metal_owned_texture_frame();
    frame.size(frame.sizeof());
    var status =
        MaplibreNativeC.mln_metal_owned_texture_acquire_frame(
            JavaCppSupport.renderSession(session), frame);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      copyMetalFrame(frame, outLongs, outInts, outDoubles);
    }
    return status;
  }

  public static int mln_metal_owned_texture_release_frame(
      long session, long[] longs, int[] ints, double[] doubles) {
    var frame = new MaplibreNativeC.mln_metal_owned_texture_frame();
    frame.size(frame.sizeof());
    frame.generation(longs[0]);
    frame.width(ints[0]);
    frame.height(ints[1]);
    frame.scale_factor(doubles[0]);
    frame.frame_id(longs[1]);
    frame.texture(JavaCppSupport.pointerOrNull(longs[2]));
    frame.device(JavaCppSupport.pointerOrNull(longs[3]));
    frame.pixel_format(longs[4]);
    return MaplibreNativeC.mln_metal_owned_texture_release_frame(
        JavaCppSupport.renderSession(session), frame);
  }

  public static int mln_vulkan_owned_texture_acquire_frame(
      long session, long[] outLongs, int[] outInts, double[] outDoubles) {
    var frame = new MaplibreNativeC.mln_vulkan_owned_texture_frame();
    frame.size(frame.sizeof());
    var status =
        MaplibreNativeC.mln_vulkan_owned_texture_acquire_frame(
            JavaCppSupport.renderSession(session), frame);
    if (status == MaplibreNativeC.MLN_STATUS_OK) {
      copyVulkanFrame(frame, outLongs, outInts, outDoubles);
    }
    return status;
  }

  public static int mln_vulkan_owned_texture_release_frame(
      long session, long[] longs, int[] ints, double[] doubles) {
    var frame = new MaplibreNativeC.mln_vulkan_owned_texture_frame();
    frame.size(frame.sizeof());
    frame.generation(longs[0]);
    frame.width(ints[0]);
    frame.height(ints[1]);
    frame.scale_factor(doubles[0]);
    frame.frame_id(longs[1]);
    frame.image(JavaCppSupport.pointerOrNull(longs[2]));
    frame.image_view(JavaCppSupport.pointerOrNull(longs[3]));
    frame.device(JavaCppSupport.pointerOrNull(longs[4]));
    frame.format(ints[2]);
    frame.layout(ints[3]);
    return MaplibreNativeC.mln_vulkan_owned_texture_release_frame(
        JavaCppSupport.renderSession(session), frame);
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

  private static void copyMetalFrame(
      MaplibreNativeC.mln_metal_owned_texture_frame frame,
      long[] outLongs,
      int[] outInts,
      double[] outDoubles) {
    outLongs[0] = frame.generation();
    outInts[0] = frame.width();
    outInts[1] = frame.height();
    outDoubles[0] = frame.scale_factor();
    outLongs[1] = frame.frame_id();
    outLongs[2] = address(frame.texture());
    outLongs[3] = address(frame.device());
    outLongs[4] = frame.pixel_format();
  }

  private static void copyVulkanFrame(
      MaplibreNativeC.mln_vulkan_owned_texture_frame frame,
      long[] outLongs,
      int[] outInts,
      double[] outDoubles) {
    outLongs[0] = frame.generation();
    outInts[0] = frame.width();
    outInts[1] = frame.height();
    outDoubles[0] = frame.scale_factor();
    outLongs[1] = frame.frame_id();
    outLongs[2] = address(frame.image());
    outLongs[3] = address(frame.image_view());
    outLongs[4] = address(frame.device());
    outInts[2] = frame.format();
    outInts[3] = frame.layout();
  }

  private static long address(org.bytedeco.javacpp.Pointer pointer) {
    return pointer == null || pointer.isNull() ? 0 : pointer.address();
  }
}
