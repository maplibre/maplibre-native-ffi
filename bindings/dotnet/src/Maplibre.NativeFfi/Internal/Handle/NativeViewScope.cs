namespace Maplibre.NativeFfi.Internal.Pointer;

internal sealed class NativeViewScope
{
    private readonly int thread = Environment.CurrentManagedThreadId;
    private bool active = true;

    internal void EnsureActive()
    {
        if (!active || thread != Environment.CurrentManagedThreadId)
            throw new MaplibreException(
                global::Maplibre.NativeFfi.Status.InvalidState,
                "This native view is valid only inside its synchronous callback."
            );
    }

    internal T Active<T>(T value)
    {
        EnsureActive();
        return value;
    }

    internal void Expire() => active = false;
}
