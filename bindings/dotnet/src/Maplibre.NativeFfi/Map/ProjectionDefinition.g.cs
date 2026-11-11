// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A style's projection at one zoom: <c>from</c> turning into <c>to</c>.
/// </summary>
/// <remarks>
/// <para>A zeroed definition is Mercator.</para>
/// <para>See <c>mln_projection_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Transition">
/// How far <c>from</c> has turned into <c>to</c>, from 0 to 1. Ignored when
/// <c>from</c> equals <c>to</c>. A value outside that range holds at the nearer
/// end.
/// </param>
public readonly partial record struct ProjectionDefinition(
    ProjectionType From,
    ProjectionType To,
    double Transition
);
