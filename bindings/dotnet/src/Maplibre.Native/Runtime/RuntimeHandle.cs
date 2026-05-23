using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Runtime;

/// <summary>Owner-thread runtime handle for MapLibre Native work and event polling.</summary>
public sealed unsafe class RuntimeHandle : IDisposable
{
    private readonly Lock callbackGate = new();
    private readonly NativeHandleState<mln_runtime> state;
    private ResourceProviderState? resourceProviderState;
    private ResourceTransformState? resourceTransformState;

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

    /// <summary>Installs or replaces the runtime-scoped resource provider callback.</summary>
    public void SetResourceProvider(ResourceProviderCallback callback)
    {
        var replacement = new ResourceProviderState(callback);
        lock (callbackGate)
        {
            try
            {
                var descriptor = replacement.Descriptor;
                NativeStatus.Check(NativeMethods.mln_runtime_set_resource_provider(Pointer, &descriptor));
                var previous = resourceProviderState;
                resourceProviderState = replacement;
                previous?.Dispose();
            }
            catch
            {
                replacement.Dispose();
                throw;
            }
        }
    }

    /// <summary>Installs or replaces the runtime-scoped resource transform callback.</summary>
    public void SetResourceTransform(ResourceTransformCallback callback)
    {
        var replacement = new ResourceTransformState(callback);
        lock (callbackGate)
        {
            try
            {
                var descriptor = replacement.Descriptor;
                NativeStatus.Check(NativeMethods.mln_runtime_set_resource_transform(Pointer, &descriptor));
                var previous = resourceTransformState;
                resourceTransformState = replacement;
                previous?.Dispose();
            }
            catch
            {
                replacement.Dispose();
                throw;
            }
        }
    }

    /// <summary>Clears the runtime-scoped resource transform callback.</summary>
    public void ClearResourceTransform()
    {
        lock (callbackGate)
        {
            NativeStatus.Check(NativeMethods.mln_runtime_clear_resource_transform(Pointer));
            var previous = resourceTransformState;
            resourceTransformState = null;
            previous?.Dispose();
        }
    }

    /// <summary>Runs one pending owner-thread task for this runtime.</summary>
    public void RunOnce()
    {
        NativeStatus.Check(NativeMethods.mln_runtime_run_once(Pointer));
    }

    /// <summary>Polls and copies the next runtime event, when one is queued.</summary>
    public RuntimeEvent? PollEvent()
    {
        var raw = RuntimeStructs.EmptyNativeEvent();
        var hasEvent = false;
        NativeStatus.Check(NativeMethods.mln_runtime_poll_event(Pointer, &raw, &hasEvent));
        return hasEvent ? RuntimeStructs.ReadEvent(raw) : null;
    }

    /// <summary>Destroys the runtime on its owner thread.</summary>
    public void Close()
    {
        state.Close();
        DisposeCallbackState();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        if (state.TryClose())
        {
            DisposeCallbackState();
        }
    }

    private void DisposeCallbackState()
    {
        lock (callbackGate)
        {
            var provider = resourceProviderState;
            var transform = resourceTransformState;
            resourceProviderState = null;
            resourceTransformState = null;
            provider?.Dispose();
            transform?.Dispose();
        }
    }
}
