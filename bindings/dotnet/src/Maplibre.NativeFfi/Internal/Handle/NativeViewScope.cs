namespace Maplibre.NativeFfi.Internal.Pointer;

internal sealed class NativeViewScope
{
    private readonly int thread = Environment.CurrentManagedThreadId;
    private bool active = true;

    internal void EnsureActive()
    {
        if (!active || thread != Environment.CurrentManagedThreadId)
            throw new InvalidOperationException(
                "This native view is valid only inside its synchronous callback."
            );
    }

    internal void Expire() => active = false;
}
