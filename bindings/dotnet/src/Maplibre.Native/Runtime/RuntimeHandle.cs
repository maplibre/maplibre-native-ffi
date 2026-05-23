using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Runtime;

/// <summary>Owner-thread runtime handle for MapLibre Native work and event polling.</summary>
public sealed unsafe class RuntimeHandle : IDisposable
{
    private readonly NativeHandleState<mln_runtime> state;

    private RuntimeHandle(mln_runtime* handle)
    {
        state = new NativeHandleState<mln_runtime>(
            handle,
            static handle => NativeMethods.mln_runtime_destroy(handle),
            nameof(RuntimeHandle));
    }

    /// <summary>Creates a runtime on the current thread.</summary>
    public static RuntimeHandle Create(RuntimeOptions? options = null)
    {
        NativeLibraryLoader.EnsureLoaded();
        options ??= new RuntimeOptions();
        using var nativeOptions = options.ToNative();
        var value = nativeOptions.Value;
        mln_runtime* runtime = null;

        NativeStatus.Check(NativeMethods.mln_runtime_create(&value, &runtime));
        return new RuntimeHandle(runtime);
    }

    internal mln_runtime* Pointer => state.Pointer;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Runs one pending owner-thread task for this runtime.</summary>
    public void RunOnce()
    {
        NativeStatus.Check(NativeMethods.mln_runtime_run_once(Pointer));
    }

    /// <summary>Destroys the runtime on its owner thread.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
    }
}
