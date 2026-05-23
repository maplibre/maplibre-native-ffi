namespace Maplibre.Native.Runtime;

/// <summary>Owner-thread offline database operation token.</summary>
public sealed class OfflineOperationHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public void Close() => IsClosed = true;
    public void Dispose() => Close();
}
