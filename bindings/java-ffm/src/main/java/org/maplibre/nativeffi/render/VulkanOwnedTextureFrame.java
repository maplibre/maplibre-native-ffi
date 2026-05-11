package org.maplibre.nativeffi.render;

/** Borrowed Vulkan texture frame valid only during the frame callback. */
public final class VulkanOwnedTextureFrame {
  private final FrameScope scope;
  private final long generation;
  private final int width;
  private final int height;
  private final double scaleFactor;
  private final long frameId;
  private final NativePointer image;
  private final NativePointer imageView;
  private final NativePointer device;
  private final int format;
  private final int layout;

  VulkanOwnedTextureFrame(
      FrameScope scope,
      long generation,
      int width,
      int height,
      double scaleFactor,
      long frameId,
      NativePointer image,
      NativePointer imageView,
      NativePointer device,
      int format,
      int layout) {
    this.scope = scope;
    this.generation = generation;
    this.width = width;
    this.height = height;
    this.scaleFactor = scaleFactor;
    this.frameId = frameId;
    this.image = image;
    this.imageView = imageView;
    this.device = device;
    this.format = format;
    this.layout = layout;
  }

  public long generation() {
    scope.ensureActive();
    return generation;
  }

  public int width() {
    scope.ensureActive();
    return width;
  }

  public int height() {
    scope.ensureActive();
    return height;
  }

  public double scaleFactor() {
    scope.ensureActive();
    return scaleFactor;
  }

  public long frameId() {
    scope.ensureActive();
    return frameId;
  }

  public NativePointer image() {
    scope.ensureActive();
    return image;
  }

  public NativePointer imageView() {
    scope.ensureActive();
    return imageView;
  }

  public NativePointer device() {
    scope.ensureActive();
    return device;
  }

  public int format() {
    scope.ensureActive();
    return format;
  }

  public int layout() {
    scope.ensureActive();
    return layout;
  }
}
