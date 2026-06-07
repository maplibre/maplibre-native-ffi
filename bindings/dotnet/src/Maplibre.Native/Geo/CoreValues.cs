namespace Maplibre.Native.Geo;

/// <summary>Geographic coordinate in degrees.</summary>
public readonly record struct LatLng(double Latitude, double Longitude);

/// <summary>Geographic bounds in degrees.</summary>
public readonly record struct LatLngBounds(LatLng Southwest, LatLng Northeast);

/// <summary>Spherical Mercator projected meters.</summary>
public readonly record struct ProjectedMeters(double Northing, double Easting);

/// <summary>Screen coordinate in logical map pixels.</summary>
public readonly record struct ScreenPoint(double X, double Y);

/// <summary>Screen-space box in logical map pixels.</summary>
public readonly record struct ScreenBox(ScreenPoint Min, ScreenPoint Max);

/// <summary>3D vector.</summary>
public readonly record struct Vec3(double X, double Y, double Z);

/// <summary>Quaternion value.</summary>
public readonly record struct Quaternion(double X, double Y, double Z, double W);

/// <summary>Overscaled tile identity.</summary>
public readonly record struct TileId(
    uint OverscaledZ,
    int Wrap,
    uint CanonicalZ,
    uint CanonicalX,
    uint CanonicalY
);

/// <summary>Canonical tile identity.</summary>
public readonly record struct CanonicalTileId(uint Z, uint X, uint Y);
