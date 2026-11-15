// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The IOSurface of an exported ring's acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_io_surface_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The surface's index in the ring.
/// </param>
/// <param name="Surface">
/// The <c>IOSurfaceRef</c>.
/// </param>
public readonly partial record struct IoSurfaceFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    NativePointer Surface
);
