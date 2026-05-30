package org.maplibre.nativeffi.render

/** Mutable Metal backend context descriptor. */
public class MetalContextDescriptor(device: NativePointer = NativePointer.NULL) {
  public var device: NativePointer = device
}
