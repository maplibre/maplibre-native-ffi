using System.Collections.Concurrent;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class NativeOwnerCallbackRoot : IDisposable
{
    private static readonly ConcurrentDictionary<
        nint,
        WeakReference<NativeOwnerCallbackRoot>
    > roots = new();
    private static long nextToken;
    private readonly object callback;
    private nint token;

    internal NativeOwnerCallbackRoot(object callback, object? owner = null)
    {
        this.callback = callback;
        Owner = owner;
        token = checked((nint)Interlocked.Increment(ref nextToken));
        if (token <= 0)
            throw new InvalidOperationException("Native callback token space exhausted.");
        roots[token] = new(this);
    }

    internal void* Pointer => (void*)token;
    internal object? Owner { get; }
    internal object Callback => callback;

    internal static object? Value(void* context) => Root(context)?.callback;

    internal static NativeOwnerCallbackRoot? Root(void* context) =>
        roots.TryGetValue((nint)context, out var weak) && weak.TryGetTarget(out var root)
            ? root
            : null;

    public void Dispose()
    {
        var current = Interlocked.Exchange(ref token, 0);
        if (current != 0)
            roots.TryRemove(current, out _);
    }
}
