package org.maplibre.nativeffi.render

/** Mutable Metal backend context descriptor. */
public class MetalContextDescriptor(device: NativePointer) {
  /**
   * Borrowed `MTLDevice`.
   *
   * Texture sessions require a non-null device. Surface attach accepts a null device and then uses
   * `MTLCreateSystemDefaultDevice()`. Surface set-target ignores a null device and keeps the
   * attached device.
   */
  public var device: NativePointer = device
}
