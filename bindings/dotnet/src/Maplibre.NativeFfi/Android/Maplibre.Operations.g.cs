// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
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
        using var call = Enter(null, "mln_android_init");
        Check(
            NativeMethods.mln_android_init(
                (void*)jniEnv.Address,
                (void*)jniClass.Address,
                (void*)context.Address,
                Diagnostic
            )
        );
    }

    public static uint CVersion()
    {
        using var call = Enter(null, "mln_c_version");
        var returned = NativeMethods.mln_c_version();
        return returned;
    }

    public static LatLng LatLngForProjectedMeters(ProjectedMeters meters)
    {
        using var call = Enter(null, "mln_lat_lng_for_projected_meters");
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_lat_lng_for_projected_meters(
                NativeProjectedMeters(meters),
                &outCoordinate,
                Diagnostic
            )
        );
        return CopyLatLng(outCoordinate);
    }

    public static void LogClearCallback()
    {
        using var call = Enter(null, "mln_log_clear_callback");
        Check(NativeMethods.mln_log_clear_callback(Diagnostic));
    }

    public static void LogSetAsyncSeverityMask(LogSeverityMask mask)
    {
        using var call = Enter(null, "mln_log_set_async_severity_mask");
        Check(NativeMethods.mln_log_set_async_severity_mask((uint)mask, Diagnostic));
    }

    public static void LogSetCallback(Func<LogSeverity, LogEvent, long, string, uint>? callback)
    {
        using var scope = new NativeCallScope(null, "mln_log_set_callback");
        Check(
            NativeMethods.mln_log_set_callback(
                callback is null ? null : &InvokeLogCallback,
                callback is null ? null : scope.Register(callback),
                &NativeCallbackRoot.Release,
                Diagnostic
            )
        );
        scope.Accept();
    }

    public static NetworkStatus NetworkStatusGet()
    {
        using var call = Enter(null, "mln_network_status_get");
        uint outStatus = default;
        Check(NativeMethods.mln_network_status_get(&outStatus, Diagnostic));
        return (NetworkStatus)outStatus;
    }

    public static void NetworkStatusSet(NetworkStatus status)
    {
        using var call = Enter(null, "mln_network_status_set");
        Check(NativeMethods.mln_network_status_set((uint)status, Diagnostic));
    }

    public static OpenglContextProviderFlag OpenglSupportedContextProviderMask()
    {
        using var call = Enter(null, "mln_opengl_supported_context_provider_mask");
        var returned = NativeMethods.mln_opengl_supported_context_provider_mask();
        return (OpenglContextProviderFlag)returned;
    }

    public static NativePointer PluginGetRegisterFunctionV1()
    {
        using var call = Enter(null, "mln_plugin_get_register_function_v1");
        var returned = NativeMethods.mln_plugin_get_register_function_v1();
        return NativePointer.FromNativeAddress((nint)returned);
    }

    public static ProjectedMeters ProjectedMetersForLatLng(LatLng coordinate)
    {
        using var call = Enter(null, "mln_projected_meters_for_lat_lng");
        var outMeters = default(mln_projected_meters);
        Check(
            NativeMethods.mln_projected_meters_for_lat_lng(
                NativeLatLng(coordinate),
                &outMeters,
                Diagnostic
            )
        );
        return CopyProjectedMeters(outMeters);
    }

    public static (uint Width, uint Height) RenderTargetExtentPhysicalSize(
        RenderTargetExtent extent
    )
    {
        using var call = Enter(null, "mln_render_target_extent_physical_size");
        var nativeExtent = NativeRenderTargetExtent(extent);
        uint outWidth = default;
        uint outHeight = default;
        Check(
            NativeMethods.mln_render_target_extent_physical_size(
                &nativeExtent,
                &outWidth,
                &outHeight,
                Diagnostic
            )
        );
        return (outWidth, outHeight);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryBox(ScreenBox box)
    {
        using var call = Enter(null, "mln_rendered_query_geometry_box");
        var returned = NativeMethods.mln_rendered_query_geometry_box(NativeScreenBox(box));
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryLineString(ScreenPoint[] points)
    {
        using var scope = new NativeCallScope(null, "mln_rendered_query_geometry_line_string");
        var returned = NativeMethods.mln_rendered_query_geometry_line_string(
            scope.Array<mln_screen_point, ScreenPoint>(points, item => NativeScreenPoint(item)),
            checked((nuint)points.Length)
        );
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderedQueryGeometry RenderedQueryGeometryPoint(ScreenPoint point)
    {
        using var call = Enter(null, "mln_rendered_query_geometry_point");
        var returned = NativeMethods.mln_rendered_query_geometry_point(NativeScreenPoint(point));
        return CopyRenderedQueryGeometry(returned);
    }

    public static RenderBackendFlag SupportedRenderBackendMask()
    {
        using var call = Enter(null, "mln_supported_render_backend_mask");
        var returned = NativeMethods.mln_supported_render_backend_mask();
        return (RenderBackendFlag)returned;
    }
}
