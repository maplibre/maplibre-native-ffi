namespace Maplibre.Native.Error;

/// <summary>Exception for owner-thread-affine operations called from another thread.</summary>
public sealed class WrongThreadException : MaplibreException
{
    public WrongThreadException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException = null
    )
        : base(status, rawStatus, diagnostic, innerException) { }
}
