namespace Maplibre.Native.Resource;

/// <summary>Resource provider request handle.</summary>
public sealed class ResourceRequestHandle : IDisposable
{
    private bool completedOrReleased;

    public bool IsClosed => completedOrReleased;

    public void Complete(ResourceResponse response)
    {
        ArgumentNullException.ThrowIfNull(response);
        ThrowIfClosed();
        throw new NotImplementedException("Resource request completion will be implemented with resource provider callbacks.");
    }

    public void Dispose()
    {
        completedOrReleased = true;
    }

    private void ThrowIfClosed()
    {
        if (completedOrReleased)
        {
            throw new ObjectDisposedException(nameof(ResourceRequestHandle));
        }
    }
}
