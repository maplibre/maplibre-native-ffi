using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Map;

/// <summary>Owner-thread map handle bound to a runtime.</summary>
public sealed unsafe class MapHandle : IDisposable
{
    private readonly RuntimeHandle runtime;
    private readonly NativeHandleState<mln_map> state;

    private MapHandle(RuntimeHandle runtime, mln_map* handle)
    {
        this.runtime = runtime;
        state = new NativeHandleState<mln_map>(
            handle,
            static handle => NativeMethods.mln_map_destroy(handle),
            nameof(MapHandle));
    }

    /// <summary>Creates a map from a runtime on the runtime owner thread.</summary>
    public static MapHandle Create(RuntimeHandle runtime, MapOptions? options = null)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        options ??= new MapOptions();
        var nativeOptions = options.ToNative();
        mln_map* map = null;

        NativeStatus.Check(NativeMethods.mln_map_create(runtime.Pointer, &nativeOptions, &map));
        return new MapHandle(runtime, map);
    }

    internal mln_map* Pointer => state.Pointer;

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Requests a repaint for a continuous map.</summary>
    public void RequestRepaint()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_repaint(Pointer));
    }

    /// <summary>Loads a style URL through MapLibre Native style APIs.</summary>
    public void SetStyleUrl(string url)
    {
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        NativeStatus.Check(NativeMethods.mln_map_set_style_url(Pointer, nativeUrl.Pointer));
    }

    /// <summary>Loads inline style JSON through MapLibre Native style APIs.</summary>
    public void SetStyleJson(string json)
    {
        ArgumentNullException.ThrowIfNull(json);
        using var nativeJson = NativeUtf8String.FromNullableString(json, nameof(json));
        NativeStatus.Check(NativeMethods.mln_map_set_style_json(Pointer, nativeJson.Pointer));
    }

    /// <summary>Destroys the map on its owner thread.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
        GC.KeepAlive(runtime);
    }
}
