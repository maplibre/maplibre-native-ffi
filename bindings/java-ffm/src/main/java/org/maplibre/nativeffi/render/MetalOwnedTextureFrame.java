package org.maplibre.nativeffi.render;

/** Borrowed Metal texture frame valid only while its frame handle is open. */
public final class MetalOwnedTextureFrame {
  private final FrameScope scope;
  private final long generation;
  private final int width;
  private final int height;
  private final double scaleFactor;
  private final long frameId;
  private final NativePointer texture;
  private final NativePointer device;
  private final long pixelFormat;

  MetalOwnedTextureFrame(
      FrameScope scope,
      long generation,
      int width,
      int height,
      double scaleFactor,
      long frameId,
      NativePointer texture,
      NativePointer device,
      long pixelFormat) {
    this.scope = scope;
    this.generation = generation;
    this.width = width;
    this.height = height;
    this.scaleFactor = scaleFactor;
    this.frameId = frameId;
    this.texture = texture;
    this.device = device;
    this.pixelFormat = pixelFormat;
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

  public NativePointer texture() {
    scope.ensureActive();
    return texture;
  }

  public NativePointer device() {
    scope.ensureActive();
    return device;
  }

  public long pixelFormat() {
    scope.ensureActive();
    return pixelFormat;
  }
}
