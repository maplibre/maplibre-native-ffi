package org.maplibre.nativejni.render;

/** CPU texture readback metadata in physical pixels. */
public record TextureImageInfo(int width, int height, int stride, long byteLength) {}
