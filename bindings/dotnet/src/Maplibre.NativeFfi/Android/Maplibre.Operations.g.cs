// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class Maplibre
{
    /// <summary>
    /// Initializes Android platform services.
    /// </summary>
    /// <remarks>
    /// See <c>mln_android_init</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Reports the C ABI contract version. The value is 0 while the ABI is
    /// unstable, and will increment on each SemVer major release.
    /// </summary>
    /// <remarks>
    /// See <c>mln_c_version</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public static uint CVersion()
    {
        using var call = Enter(null, "mln_c_version");
        var returned = NativeMethods.mln_c_version();
        return returned;
    }

    /// <summary>
    /// Converts spherical Mercator projected meters to a geographic coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_lat_lng_for_projected_meters</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Clears the process-global log callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_clear_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void LogClearCallback()
    {
        using var call = Enter(null, "mln_log_clear_callback");
        Check(NativeMethods.mln_log_clear_callback(Diagnostic));
    }

    /// <summary>
    /// Controls which log severities MapLibre Native may dispatch
    /// asynchronously.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_set_async_severity_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void LogSetAsyncSeverityMask(LogSeverityMask mask)
    {
        using var call = Enter(null, "mln_log_set_async_severity_mask");
        Check(NativeMethods.mln_log_set_async_severity_mask((uint)mask, Diagnostic));
    }

    /// <summary>
    /// Installs a process-global MapLibre Native log callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_set_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void LogSetCallback(LogHandler handler)
    {
        using var scope = new NativeCallScope(null, "mln_log_set_callback");
        var nativeHandler = NativeLogHandler(handler, scope);
        Check(NativeMethods.mln_log_set_callback(&nativeHandler, Diagnostic));
        scope.Accept();
    }

    /// <summary>
    /// Reads MapLibre Native's process-global network status.
    /// </summary>
    /// <remarks>
    /// See <c>mln_network_get_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static NetworkStatus NetworkGetStatus()
    {
        using var call = Enter(null, "mln_network_get_status");
        uint outStatus = default;
        Check(NativeMethods.mln_network_get_status(&outStatus, Diagnostic));
        return (NetworkStatus)outStatus;
    }

    /// <summary>
    /// Sets MapLibre Native's process-global network status.
    /// </summary>
    /// <remarks>
    /// See <c>mln_network_set_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public static void NetworkSetStatus(NetworkStatus status)
    {
        using var call = Enter(null, "mln_network_set_status");
        Check(NativeMethods.mln_network_set_status((uint)status, Diagnostic));
    }

    /// <summary>
    /// Returns OpenGL context providers supported by this build.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_supported_context_provider_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
    /// </remarks>
    public static OpenglContextProviderFlag OpenglSupportedContextProviderMask()
    {
        using var call = Enter(null, "mln_opengl_supported_context_provider_mask");
        var returned = NativeMethods.mln_opengl_supported_context_provider_mask();
        return (OpenglContextProviderFlag)returned;
    }

    /// <summary>
    /// Returns the process-wide <c>mln_plugin_register_v1</c> entry point;
    /// never null.
    /// </summary>
    /// <remarks>
    /// See <c>mln_plugin_get_register_function_v1</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html">C API reference</see>.
    /// </remarks>
    public static NativePointer PluginGetRegisterFunctionV1()
    {
        using var call = Enter(null, "mln_plugin_get_register_function_v1");
        var returned = NativeMethods.mln_plugin_get_register_function_v1();
        return NativePointer.FromNativeAddress((nint)returned);
    }

    /// <summary>
    /// Converts a geographic coordinate to spherical Mercator projected meters.
    /// </summary>
    /// <remarks>
    /// See <c>mln_projected_meters_for_lat_lng</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Computes the physical device-pixel size of a logical render target
    /// extent.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_target_extent_physical_size</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Returns a rendered box query geometry descriptor.
    /// </summary>
    /// <remarks>
    /// See <c>mln_rendered_query_geometry_box</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public static RenderedQueryGeometry RenderedQueryGeometryBox(ScreenBox box)
    {
        using var call = Enter(null, "mln_rendered_query_geometry_box");
        var returned = NativeMethods.mln_rendered_query_geometry_box(NativeScreenBox(box));
        return CopyRenderedQueryGeometry(returned);
    }

    /// <summary>
    /// Returns a rendered line-string query geometry descriptor.
    /// </summary>
    /// <remarks>
    /// See <c>mln_rendered_query_geometry_line_string</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public static RenderedQueryGeometry RenderedQueryGeometryLineString(ScreenPoint[] points)
    {
        using var scope = new NativeCallScope(null, "mln_rendered_query_geometry_line_string");
        var returned = NativeMethods.mln_rendered_query_geometry_line_string(
            scope.Array<mln_screen_point, ScreenPoint>(points, item => NativeScreenPoint(item)),
            checked((nuint)points.Length)
        );
        return CopyRenderedQueryGeometry(returned);
    }

    /// <summary>
    /// Returns a rendered point query geometry descriptor.
    /// </summary>
    /// <remarks>
    /// See <c>mln_rendered_query_geometry_point</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public static RenderedQueryGeometry RenderedQueryGeometryPoint(ScreenPoint point)
    {
        using var call = Enter(null, "mln_rendered_query_geometry_point");
        var returned = NativeMethods.mln_rendered_query_geometry_point(NativeScreenPoint(point));
        return CopyRenderedQueryGeometry(returned);
    }

    /// <summary>
    /// Reports the render backends available in this native library build.
    /// </summary>
    /// <remarks>
    /// See <c>mln_supported_render_backend_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
    /// </remarks>
    public static RenderBackendFlag SupportedRenderBackendMask()
    {
        using var call = Enter(null, "mln_supported_render_backend_mask");
        var returned = NativeMethods.mln_supported_render_backend_mask();
        return (RenderBackendFlag)returned;
    }
}
