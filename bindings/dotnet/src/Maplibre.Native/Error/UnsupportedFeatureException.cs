namespace Maplibre.Native.Error;

/// <summary>Exception for unavailable platforms, backends, entry points, or behaviors.</summary>
public sealed class UnsupportedFeatureException : MaplibreException
{
    public UnsupportedFeatureException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException
    )
        : base(status, rawStatus, diagnostic, innerException) { }
}
