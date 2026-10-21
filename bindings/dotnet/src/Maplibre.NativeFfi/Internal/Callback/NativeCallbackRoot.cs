using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class NativeCallbackRoot : IDisposable
{
    private readonly object value;
    private nint token;

    internal NativeCallbackRoot(object value)
    {
        this.value = value;
        token = GCHandle.ToIntPtr(GCHandle.Alloc(this));
    }

    internal void* Pointer => (void*)token;

    private static NativeCallbackRoot Root(void* context) =>
        (NativeCallbackRoot)GCHandle.FromIntPtr((nint)context).Target!;

    internal static object Value(void* context) => Root(context).value;

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    internal static void Release(void* context) => Root(context).Dispose();

    public void Dispose()
    {
        var released = Interlocked.Exchange(ref token, 0);
        if (released != 0)
            GCHandle.FromIntPtr(released).Free();
    }
}
