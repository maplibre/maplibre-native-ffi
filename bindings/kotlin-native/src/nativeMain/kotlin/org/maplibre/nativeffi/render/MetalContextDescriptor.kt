package org.maplibre.nativeffi.render

/** Mutable Metal backend context descriptor. */
public class MetalContextDescriptor(public var device: NativePointer = NativePointer.NULL) {
  public fun device(device: NativePointer): MetalContextDescriptor = apply { this.device = device }
}
