// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The screen area that a rendered feature query covers.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_query_geometry</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public abstract record RenderedQueryGeometry
{
    private RenderedQueryGeometry() { }

    public sealed record Point(ScreenPoint Value) : RenderedQueryGeometry;

    public sealed record Box(ScreenBox Value) : RenderedQueryGeometry;

    public sealed record LineString(ScreenLineString Value) : RenderedQueryGeometry;
}
