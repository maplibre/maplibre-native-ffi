package org.maplibre.nativeffi.render;

/** CPU texture readback metadata in physical pixels. */
public record TextureImageInfo(int width, int height, int stride, long byteLength) {}
