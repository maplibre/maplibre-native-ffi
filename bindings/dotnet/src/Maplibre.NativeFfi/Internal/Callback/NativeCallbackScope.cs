namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class NativeCallbackScope<T>(T* pointer)
    where T : unmanaged
{
    private readonly int thread = Environment.CurrentManagedThreadId;
    private bool active = true;

    internal T* Pointer
    {
        get
        {
            if (!active || thread != Environment.CurrentManagedThreadId)
                throw new InvalidOperationException(
                    "This response is only valid inside its native callback."
                );
            return pointer;
        }
    }

    internal void Expire() => active = false;
}
