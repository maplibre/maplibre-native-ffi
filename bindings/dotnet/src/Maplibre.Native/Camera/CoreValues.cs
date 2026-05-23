namespace Maplibre.Native.Camera;

/// <summary>Camera edge insets in logical pixels.</summary>
public readonly record struct EdgeInsets(double Top, double Left, double Bottom, double Right);

/// <summary>Cubic Bézier control points.</summary>
public readonly record struct UnitBezier(double X1, double Y1, double X2, double Y2);
