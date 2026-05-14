package org.maplibre.nativeffi.render;

import java.util.Objects;

/** Mutable Metal backend context descriptor. */
public final class MetalContextDescriptor {
  private NativePointer device = NativePointer.NULL;

  public MetalContextDescriptor() {}

  public MetalContextDescriptor(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
  }

  public NativePointer device() {
    return device;
  }

  public MetalContextDescriptor device(NativePointer device) {
    this.device = Objects.requireNonNull(device, "device");
    return this;
  }
}
