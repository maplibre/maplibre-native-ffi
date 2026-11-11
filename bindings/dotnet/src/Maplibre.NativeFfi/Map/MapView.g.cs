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
/// <param name="Projection">
/// The style's projection at the camera's zoom.
/// </param>
public readonly partial record struct MapView(
    Camera Camera,
    LogicalExtent Extent,
    ProjectionMode ProjectionMode,
    MapViewport Viewport,
    ProjectionDefinition Projection
);
