package org.maplibre.nativeffi.examples.lwjglmap;

interface RenderTarget extends AutoCloseable {
  default boolean needsReattachOnResize() {
    return false;
  }

  default boolean needsMetalAutoreleasePool() {
    return false;
  }

  default void reattach(Viewport viewport) {
    throw new IllegalStateException("render target does not support reattachment");
  }

  void resize(Viewport viewport);

  void renderUpdate();

  @Override
  void close();
}
