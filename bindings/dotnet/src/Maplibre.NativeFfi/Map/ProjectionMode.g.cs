// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map's whole projection mode.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of
/// <c>ProjectionModeOptions</c>.</para>
/// <para>See <c>mln_projection_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct ProjectionMode(bool Axonometric, double XSkew, double YSkew);
