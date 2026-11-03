// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map's whole viewport.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of
/// <c>MapViewportOptions</c>.</para>
/// <para>See <c>mln_map_viewport</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MapViewport(
    NorthOrientation NorthOrientation,
    ConstrainMode ConstrainMode,
    ViewportMode ViewportMode,
    EdgeInsets FrustumOffset
);
