namespace Maplibre.NativeFfi;

/// <summary>A handle that the collector reclaimed before it was closed or disposed.</summary>
public sealed class LeakReportedEventArgs : EventArgs
{
    internal LeakReportedEventArgs(string typeName, string message)
    {
        TypeName = typeName;
        Message = message;
    }

    /// <summary>The handle's type, such as <c>MapHandle</c>.</summary>
    public string TypeName { get; }

    /// <summary>What leaked, and whether the binding could dispose it.</summary>
    public string Message { get; }
}
