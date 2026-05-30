package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.system.MemoryUtil.NULL;

import org.maplibre.nativeffi.render.NativePointer;

final class MetalBorrowedTexture implements AutoCloseable {
  private final MetalContext context;
  private long texture;

  MetalBorrowedTexture(MetalContext context, Viewport viewport) {
    this.context = context;
    this.texture = context.createBorrowedTexture(viewport);
  }

  NativePointer pointer() {
    return NativePointer.ofAddress(texture);
  }

  long texture() {
    return texture;
  }

  @Override
  public void close() {
    if (texture != NULL) {
      context.releaseObject(texture);
      texture = NULL;
    }
  }
}
