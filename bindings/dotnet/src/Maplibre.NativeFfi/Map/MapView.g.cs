// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Everything that a projection depends on: the transform that a published
/// snapshot or a rendered frame describes.
/// </summary>
/// <remarks>
/// See <c>mln_map_view</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Camera">
/// The camera. Its center is unwrapped: the longitude keeps the visible world
/// copy and may fall outside -180 to 180.
/// </param>
/// <param name="Extent">
/// Logical size and device-pixel scale. A snapshot carries the map's scale
/// factor, and a frame carries the scale factor of the target that drew it.
/// </param>
/// <param name="ProjectionMode">
/// Axonometric mode and skew, with every field set.
/// </param>
/// <param name="Viewport">
/// North orientation, constrain mode, viewport mode, and frustum offset, with
/// every field set.
/// </param>
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
