package org.maplibre.nativejni.render;

import java.util.Objects;

/** Mutable EGL context descriptor for OpenGL render targets. */
public final class EglContextDescriptor implements OpenGLContextDescriptor {
  private NativePointer display = NativePointer.NULL;
  private NativePointer config = NativePointer.NULL;
  private NativePointer shareContext = NativePointer.NULL;
  private NativePointer getProcAddress = NativePointer.NULL;

  public EglContextDescriptor() {}

  public EglContextDescriptor(
      NativePointer display, NativePointer config, NativePointer shareContext) {
    this.display = Objects.requireNonNull(display, "display");
    this.config = Objects.requireNonNull(config, "config");
    this.shareContext = Objects.requireNonNull(shareContext, "shareContext");
  }

  public NativePointer display() {
    return display;
  }

  public EglContextDescriptor display(NativePointer display) {
    this.display = Objects.requireNonNull(display, "display");
    return this;
  }

  public NativePointer config() {
    return config;
  }

  public EglContextDescriptor config(NativePointer config) {
    this.config = Objects.requireNonNull(config, "config");
    return this;
  }

  public NativePointer shareContext() {
    return shareContext;
  }

  public EglContextDescriptor shareContext(NativePointer shareContext) {
    this.shareContext = Objects.requireNonNull(shareContext, "shareContext");
    return this;
  }

  public NativePointer getProcAddress() {
    return getProcAddress;
  }

  public EglContextDescriptor getProcAddress(NativePointer getProcAddress) {
    this.getProcAddress = Objects.requireNonNull(getProcAddress, "getProcAddress");
    return this;
  }
}
