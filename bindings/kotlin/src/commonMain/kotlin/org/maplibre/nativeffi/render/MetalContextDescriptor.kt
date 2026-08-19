package org.maplibre.nativeffi.render

/** Metal backend context fields shared by Metal render targets. */
public class MetalContextDescriptor(device: NativePointer) {
  /**
   * Borrowed `MTLDevice`.
   *
   * Texture sessions require a non-null device. A Metal surface attach accepts a null device and
   * renders with the device `MTLCreateSystemDefaultDevice()` returns. A surface set-target call
   * treats a null device as naming none and keeps the device the session attached with.
   */
  public var device: NativePointer = device
}
