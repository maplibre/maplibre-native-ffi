package org.maplibre.nativeffi.render

/** Mutable Metal backend context descriptor. */
public class MetalContextDescriptor(device: NativePointer) {
  public var device: NativePointer = device
}
