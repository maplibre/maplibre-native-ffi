package org.maplibre.nativejni.render;

/** Borrowed OpenGL texture frame valid only while its frame handle is open. */
public final class OpenGLOwnedTextureFrame {
  private final FrameScope scope;
  private final long generation;
  private final int width;
  private final int height;
  private final double scaleFactor;
  private final long frameId;
  private final int texture;
  private final int target;
  private final int internalFormat;
  private final int format;
  private final int type;

  OpenGLOwnedTextureFrame(
      FrameScope scope,
      long generation,
      int width,
      int height,
      double scaleFactor,
      long frameId,
      int texture,
      int target,
      int internalFormat,
      int format,
      int type) {
    this.scope = scope;
    this.generation = generation;
    this.width = width;
    this.height = height;
    this.scaleFactor = scaleFactor;
    this.frameId = frameId;
    this.texture = texture;
    this.target = target;
    this.internalFormat = internalFormat;
    this.format = format;
    this.type = type;
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

  public int texture() {
    scope.ensureActive();
    return texture;
  }

  public int target() {
    scope.ensureActive();
    return target;
  }

  public int internalFormat() {
    scope.ensureActive();
    return internalFormat;
  }

  public int format() {
    scope.ensureActive();
    return format;
  }

  public int type() {
    scope.ensureActive();
    return type;
  }
}
