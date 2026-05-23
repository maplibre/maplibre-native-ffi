namespace Maplibre.Native.Map;

/// <summary>Owner-thread projection snapshot handle.</summary>
public sealed class MapProjectionHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public void Close() => IsClosed = true;
    public void Dispose() => Close();
}
