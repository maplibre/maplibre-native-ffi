// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One view of a map: its camera, size, and projection.
/// </summary>
/// <remarks>
/// See <c>mln_map_view</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MapView(
    Camera Camera,
    LogicalExtent Extent,
    ProjectionMode ProjectionMode,
    MapViewportOptions Viewport
)
{
    public MapView()
        : this(default, default, new(), new()) { }
}
