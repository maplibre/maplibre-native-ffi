// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A timing curve from (0, 0) to (1, 1), with the control points (<c>x1</c>,
/// <c>y1</c>) and (<c>x2</c>, <c>y2</c>).
/// </summary>
/// <remarks>
/// <para><c>x1</c> and <c>x2</c> are between 0 and 1.</para>
/// <para>See <c>mln_unit_bezier</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct UnitBezier(double X1, double Y1, double X2, double Y2);
