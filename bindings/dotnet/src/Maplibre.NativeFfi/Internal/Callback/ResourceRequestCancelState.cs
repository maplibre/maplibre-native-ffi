using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.Callback;

/// <summary>
/// Roots one host cancel callback for a resource request and dispatches the
/// native cancellation notification to it.
/// </summary>
/// <remarks>
/// The native side passes a token rather than a <see cref="GCHandle" />, so a
/// registration the binding already retired resolves to nothing instead of to
/// freed memory. Dispatch takes no request lock, which lets the host callback
/// complete or close the same request while another thread releases it.
/// </remarks>
internal sealed unsafe class ResourceRequestCancelState
{
    private static readonly Lock RegistryGate = new();
    private static readonly Dictionary<nint, ResourceRequestCancelState> Registry = [];
    private static nint nextToken;

    [ThreadStatic]
    private static int dispatchDepth;

    private readonly nint token;
    private readonly Action callback;
    private bool retired;

    internal ResourceRequestCancelState(Action callback)
    {
        this.callback = callback ?? throw new ArgumentNullException(nameof(callback));
        lock (RegistryGate)
        {
            token = ++nextToken;
            if (token == 0)
            {
                token = ++nextToken;
            }

            Registry.Add(token, this);
        }
    }

    internal static delegate* unmanaged[Cdecl]<void*, void> NativeCallback => &OnCancel;

    /// <summary>
    /// Whether the calling thread is running a host cancel callback.
    /// </summary>
    /// <remarks>
    /// Native release returns without waiting when the cancel callback calls it,
    /// so a request released from inside the callback skips the wait that a
    /// release on another thread would otherwise impose.
    /// </remarks>
    internal static bool IsDispatchingOnCurrentThread => dispatchDepth > 0;

    internal void* UserData => (void*)token;

    internal nint TokenForTest => token;

    internal bool IsRetiredForTest => retired;

    internal static void DispatchForTest(nint token) => Dispatch((void*)token);

    internal void Retire()
    {
        lock (RegistryGate)
        {
            if (retired)
            {
                return;
            }

            retired = true;
            Registry.Remove(token);
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void OnCancel(void* userData) => Dispatch(userData);

    private static void Dispatch(void* userData)
    {
        dispatchDepth++;
        try
        {
            Enter((nint)userData)?.Invoke();
        }
        catch
        {
            // A host failure must not unwind into the MapLibre cancel path.
        }
        finally
        {
            dispatchDepth--;
        }
    }

    private static ResourceRequestCancelState? Enter(nint token)
    {
        lock (RegistryGate)
        {
            return Registry.TryGetValue(token, out var state) && !state.retired ? state : null;
        }
    }

    private void Invoke()
    {
        try
        {
            callback();
        }
        catch
        {
            // A host failure must not unwind into the MapLibre cancel path.
        }
    }
}
