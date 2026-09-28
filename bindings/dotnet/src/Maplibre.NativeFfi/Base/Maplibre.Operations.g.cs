// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class Maplibre
{
    public static void AndroidInit(
        NativePointer jniEnv,
        NativePointer jniClass,
        NativePointer context
    )
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_android_init"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(
            NativeMethods.mln_android_init(
                (void*)jniEnv.Address,
                (void*)jniClass.Address,
                (void*)context.Address
            )
        );
    }

    public static uint CVersion()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_c_version"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_c_version();
        return returned;
    }

    public static LatLng LatLngForProjectedMeters(ProjectedMeters meters)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_lat_lng_for_projected_meters"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var outCoordinate = default(mln_lat_lng);
        NativeStatus.Check(
            NativeMethods.mln_lat_lng_for_projected_meters(
                NativeProjectedMeters(meters),
                &outCoordinate
            )
        );
        return CopyLatLng(outCoordinate);
    }

    public static void LogClearCallback()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_log_clear_callback"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_log_clear_callback());
    }

    public static void LogSetAsyncSeverityMask(LogSeverityMask mask)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_log_set_async_severity_mask"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_log_set_async_severity_mask((uint)mask));
    }

    public static void LogSetCallback(Func<LogSeverity, LogEvent, long, string, uint>? callback)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_log_set_callback"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var rootCallback = callback is null ? null : scope.Register(callback);
        NativeStatus.Check(
            NativeMethods.mln_log_set_callback(
                callback is null ? null : &InvokeLogCallback,
                rootCallback,
                &global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackRoot.Release
            )
        );
        scope.Accept();
    }

    public static NetworkStatus NetworkStatusGet()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_network_status_get"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        uint outStatus = default;
        NativeStatus.Check(NativeMethods.mln_network_status_get(&outStatus));
        return (NetworkStatus)outStatus;
    }

    public static void NetworkStatusSet(NetworkStatus status)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_network_status_set"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_network_status_set((uint)status));
    }

    public static OpenglContextProviderFlag OpenglSupportedContextProviderMask()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_opengl_supported_context_provider_mask"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_opengl_supported_context_provider_mask();
        return (OpenglContextProviderFlag)returned;
    }

    public static NativePointer PluginGetRegisterFunctionV1()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_plugin_get_register_function_v1"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_plugin_get_register_function_v1();
        return NativePointer.FromNativeAddress((nint)returned);
    }

    public static ProjectedMeters ProjectedMetersForLatLng(LatLng coordinate)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_projected_meters_for_lat_lng"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var outMeters = default(mln_projected_meters);
        NativeStatus.Check(
            NativeMethods.mln_projected_meters_for_lat_lng(NativeLatLng(coordinate), &outMeters)
        );
        return CopyProjectedMeters(outMeters);
    }

    public static (uint Width, uint Height) RenderTargetExtentPhysicalSize(
        RenderTargetExtent extent
    )
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_render_target_extent_physical_size"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var nativeExtent = NativeRenderTargetExtent(extent);
        uint outWidth = default;
        uint outHeight = default;
        NativeStatus.Check(
            NativeMethods.mln_render_target_extent_physical_size(
                &nativeExtent,
                &outWidth,
                &outHeight
            )
        );
        return (outWidth, outHeight);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryBox(ScreenBox box)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_rendered_query_geometry_box"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_rendered_query_geometry_box(NativeScreenBox(box));
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryLineString(ScreenPoint[] points)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_rendered_query_geometry_line_string"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_rendered_query_geometry_line_string(
            scope.Array<mln_screen_point, ScreenPoint>(points, item => NativeScreenPoint(item)),
            checked((nuint)points.Length)
        );
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryPoint(ScreenPoint point)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_rendered_query_geometry_point"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_rendered_query_geometry_point(NativeScreenPoint(point));
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderBackendFlag SupportedRenderBackendMask()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_supported_render_backend_mask"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_supported_render_backend_mask();
        return (RenderBackendFlag)returned;
    }

    public static string ThreadLastErrorMessage()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_thread_last_error_message"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        var returned = NativeMethods.mln_thread_last_error_message();
        return NativeCallScope.CopyCString(returned);
    }
}
