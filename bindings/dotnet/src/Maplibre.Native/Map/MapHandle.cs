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

    /// <summary>Requests an asynchronous still-image render for a static map.</summary>
    public void RequestStillImage()
    {
        NativeStatus.Check(NativeMethods.mln_map_request_still_image(Pointer));
    }

    /// <summary>Sets native debug drawing options.</summary>
    public void SetDebugOptions(DebugOptions options)
    {
        NativeStatus.Check(NativeMethods.mln_map_set_debug_options(Pointer, (uint)options));
    }

    /// <summary>Gets native debug drawing options.</summary>
    public DebugOptions GetDebugOptions()
    {
        uint options = 0;
        NativeStatus.Check(NativeMethods.mln_map_get_debug_options(Pointer, &options));
        return (DebugOptions)options;
    }

    /// <summary>Shows or hides the built-in rendering statistics overlay.</summary>
    public void SetRenderingStatsViewEnabled(bool enabled)
    {
        NativeStatus.Check(NativeMethods.mln_map_set_rendering_stats_view_enabled(Pointer, enabled ? (byte)1 : (byte)0));
    }

    /// <summary>Whether the built-in rendering statistics overlay is enabled.</summary>
    public bool GetRenderingStatsViewEnabled()
    {
        bool enabled = false;
        NativeStatus.Check(NativeMethods.mln_map_get_rendering_stats_view_enabled(Pointer, &enabled));
        return enabled;
    }

    /// <summary>Whether the native map reports all required resources loaded.</summary>
    public bool IsFullyLoaded()
    {
        bool loaded = false;
        NativeStatus.Check(NativeMethods.mln_map_is_fully_loaded(Pointer, &loaded));
        return loaded;
    }

    /// <summary>Asks the native map to write debug logs through the native log system.</summary>
    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_map_dump_debug_logs(Pointer));
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
