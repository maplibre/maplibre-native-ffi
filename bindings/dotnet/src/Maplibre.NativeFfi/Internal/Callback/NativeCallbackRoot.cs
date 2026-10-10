using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class NativeCallbackRoot : IDisposable
{
    private static readonly ConcurrentDictionary<nint, WeakReference<NativeCallbackRoot>> roots =
        new();
    private static long nextToken;
    private readonly object gate = new();
    private readonly nint token;
    private object? value;
    private NativeCallbackOwner? owner;
    internal NativeCallbackRoot? Previous;
    internal NativeCallbackRoot? Next;

    internal NativeCallbackRoot(object value)
    {
        this.value = value;
        token = checked((nint)Interlocked.Increment(ref nextToken));
        if (token <= 0)
            throw new InvalidOperationException("Native callback token space exhausted.");
        // Owner finalization transfers live roots to the retirement list.
        roots[token] = new(this, trackResurrection: true);
    }

    internal void* Pointer => (void*)token;

    internal static object Value(void* context)
    {
        if (roots.TryGetValue((nint)context, out var weak) && weak.TryGetTarget(out var root))
        {
            lock (root.gate)
                if (root.value is { } value)
                    return value;
        }
        throw new ObjectDisposedException(nameof(NativeCallbackRoot));
    }

    internal void Retain(NativeCallbackOwner destination)
    {
        lock (gate)
        {
            if (value is null || ReferenceEquals(owner, destination))
                return;
            owner?.Remove(this);
            owner = destination.Add(this);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    internal static void Release(void* context)
    {
        try
        {
            if (roots.TryRemove((nint)context, out var weak) && weak.TryGetTarget(out var root))
                root.Dispose();
        }
        catch { }
    }

    public void Dispose()
    {
        roots.TryRemove(token, out _);
        lock (gate)
        {
            value = null;
            owner?.Remove(this);
            owner = null;
        }
    }
}
