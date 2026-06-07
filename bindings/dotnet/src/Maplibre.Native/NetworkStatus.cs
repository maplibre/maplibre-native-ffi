namespace Maplibre.Native;

/// <summary>MapLibre Native process-global network reachability state.</summary>
public sealed record class NetworkStatus
{
    private NetworkStatus(uint rawValue, string name)
    {
        RawValue = rawValue;
        Name = name;
    }

    /// <summary>Allows HTTP and HTTPS requests.</summary>
    public static NetworkStatus Online { get; } = new(1, nameof(Online));

    /// <summary>Prevents new online source network requests.</summary>
    public static NetworkStatus Offline { get; } = new(2, nameof(Offline));

    /// <summary>The raw C <c>mln_network_status</c> value.</summary>
    public uint RawValue { get; }

    /// <summary>Whether this value is a known status in this binding version.</summary>
    public bool IsKnown => ReferenceEquals(this, Online) || ReferenceEquals(this, Offline);

    private string Name { get; }

    /// <summary>Creates a .NET network status value from a raw C value.</summary>
    public static NetworkStatus FromRaw(uint rawValue) =>
        rawValue switch
        {
            1 => Online,
            2 => Offline,
            _ => new NetworkStatus(rawValue, $"Unknown({rawValue})"),
        };

    /// <inheritdoc />
    public override string ToString() => Name;
}
