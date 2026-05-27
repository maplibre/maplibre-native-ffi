package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable WGL context descriptor for OpenGL render targets on Windows. */
public final class WglContextDescriptor implements OpenGLContextDescriptor {
  private NativePointer deviceContext = NativePointer.NULL;
  private NativePointer shareContext = NativePointer.NULL;
  private NativePointer getProcAddress = NativePointer.NULL;

  public WglContextDescriptor() {}

  public WglContextDescriptor(NativePointer deviceContext, NativePointer shareContext) {
    this.deviceContext = Objects.requireNonNull(deviceContext, "deviceContext");
    this.shareContext = Objects.requireNonNull(shareContext, "shareContext");
  }

  public NativePointer deviceContext() {
    return deviceContext;
  }

  public WglContextDescriptor deviceContext(NativePointer deviceContext) {
    this.deviceContext = Objects.requireNonNull(deviceContext, "deviceContext");
    return this;
  }

  public NativePointer shareContext() {
    return shareContext;
  }

  public WglContextDescriptor shareContext(NativePointer shareContext) {
    this.shareContext = Objects.requireNonNull(shareContext, "shareContext");
    return this;
  }

  public NativePointer getProcAddress() {
    return getProcAddress;
  }

  public WglContextDescriptor getProcAddress(NativePointer getProcAddress) {
    this.getProcAddress = Objects.requireNonNull(getProcAddress, "getProcAddress");
    return this;
  }
}
