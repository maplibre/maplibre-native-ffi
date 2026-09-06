using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Resource;

namespace Maplibre.NativeFfi.Internal.Callback;

/// <summary>
/// Resolves the token that crosses C as cancel callback user data back to the
/// request handle that registered it.
/// </summary>
/// <remarks>
/// The request handle owns its cancel callback. This registry holds only a weak
/// reference per token, so a registration keeps neither the handle nor its
/// callback reachable, and a token that native reports after the handle is
/// gone resolves to nothing instead of to freed memory. The registry lock is a
/// leaf lock: it protects the dictionary and nothing else, so a request handle
/// may call into the registry while holding its own lock.
/// </remarks>
internal static unsafe class ResourceRequestCancelRegistry
{
    private static readonly Lock Gate = new();
    private static readonly Dictionary<nint, WeakReference<ResourceRequestHandle>> Entries = [];
    private static nint nextToken;

    internal static delegate* unmanaged[Cdecl]<void*, void> NativeCallback => &OnCancel;

    internal static nint Register(ResourceRequestHandle handle)
    {
        lock (Gate)
        {
            var token = ++nextToken;
            if (token == 0)
            {
                token = ++nextToken;
            }

            Entries.Add(token, new WeakReference<ResourceRequestHandle>(handle));
            return token;
        }
    }

    internal static void Remove(nint token)
    {
        if (token == 0)
        {
            return;
        }

        lock (Gate)
        {
            Entries.Remove(token);
        }
    }

    internal static bool IsRegisteredForTest(nint token)
    {
        lock (Gate)
        {
            return Entries.ContainsKey(token);
        }
    }

    internal static void DispatchForTest(nint token) => Dispatch(token);

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void OnCancel(void* userData) => Dispatch((nint)userData);

    private static void Dispatch(nint token)
    {
        try
        {
            Resolve(token)?.DispatchCancel(token);
        }
        catch
        {
            // A host failure must not unwind into the MapLibre cancel path.
        }
    }

    private static ResourceRequestHandle? Resolve(nint token)
    {
        lock (Gate)
        {
            return Entries.TryGetValue(token, out var entry) && entry.TryGetTarget(out var handle)
                ? handle
                : null;
        }
    }
}
