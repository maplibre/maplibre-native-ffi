namespace Maplibre.Native.Error;

/// <summary>Base exception for MapLibre Native binding failures.</summary>
public class MaplibreException : Exception
{
    /// <summary>Creates an exception for a native or binding status failure.</summary>
    public MaplibreException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException = null
    )
        : base(MessageFor(status, rawStatus, diagnostic), innerException)
    {
        Status = status;
        RawStatus = rawStatus;
        Diagnostic = diagnostic;
    }

    /// <summary>The stable binding status category.</summary>
    public MaplibreStatus Status { get; }

    /// <summary>The raw C status value, when the failure came from a C status.</summary>
    public int? RawStatus { get; }

    /// <summary>The diagnostic copied immediately after the failing call.</summary>
    public string Diagnostic { get; }

    private static string MessageFor(MaplibreStatus status, int? rawStatus, string diagnostic)
    {
        if (rawStatus is { } value)
        {
            return $"MapLibre Native status {value} ({status}): {diagnostic}";
        }

        return diagnostic;
    }
}
