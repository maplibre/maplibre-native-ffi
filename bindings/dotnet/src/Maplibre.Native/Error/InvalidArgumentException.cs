namespace Maplibre.Native.Error;

/// <summary>Exception for invalid native or binding arguments.</summary>
public sealed class InvalidArgumentException : MaplibreException
{
    public InvalidArgumentException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException = null
    )
        : base(status, rawStatus, diagnostic, innerException) { }
}
