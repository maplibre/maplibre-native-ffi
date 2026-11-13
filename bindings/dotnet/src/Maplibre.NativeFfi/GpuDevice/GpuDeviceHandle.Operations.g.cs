// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A GPU, and the queue that render sessions on it share.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_device</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class GpuDeviceHandle : IDisposable, INativeOwner<MlnGpuDevice>
{
    private readonly NativeHandleState<MlnGpuDevice> state;

    internal GpuDeviceHandle(MlnGpuDevice handle)
    {
        state = new(handle, Abandon, nameof(GpuDeviceHandle), Abandon);
    }

    internal static GpuDeviceHandle Adopt(MlnGpuDevice handle) =>
        NativeHandleState<MlnGpuDevice>.Adopt(handle, () => new GpuDeviceHandle(handle), Abandon);

    private static mln_status Abandon(MlnGpuDevice live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_gpu_device_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGpuDevice> INativeOwner<MlnGpuDevice>.State => state;
    internal MlnGpuDevice Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    void IDisposable.Dispose()
    {
        state.Close();
    }

    /// <summary>
    /// Creates a GPU device that the library owns, for render sessions to
    /// share.
    /// </summary>
    /// <remarks>
    /// <para>The sessions on a device share its GPU and queue: one
    /// <c>VkDevice</c> and queue, one <c>MTLDevice</c> and command queue, or
    /// one OpenGL share group. <c>MapHandle.Attach()</c> creates a device of
    /// its own when it gets none. Create one to share it among the sessions of
    /// many maps, or to pick the backend or the GPU. A Vulkan device enables
    /// the presentation, timeline semaphore, external memory, and external
    /// semaphore extensions that its GPU offers.</para>
    /// <para>See <c>mln_gpu_device_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when this build or platform
    /// lacks <c>backend</c>, when the device can't render with
    /// <c>open_gl_client_api</c>, when options name a GPU that the backend
    /// can't select (an OpenGL device takes no <c>adapter_index</c>,
    /// <c>adapter_uuid</c>, or <c>adapter_luid</c>, and only a Vulkan device
    /// takes <c>adapter_uuid</c> or <c>adapter_luid</c>), when a device that
    /// doesn't render with Vulkan gets a <c>queue_lock</c>, or in a browser for
    /// every backend: WebGPU makes devices asynchronously, and a WebGL2 context
    /// needs a canvas. Attach without a device, which the session makes itself,
    /// or wrap your WebGL2 context.</description></item>
    /// <item><description><c>Status.NotFound</c> when no GPU matches
    /// <c>adapter_index</c>, <c>adapter_uuid</c>, or
    /// <c>adapter_luid</c>.</description></item>
    /// <item><description><c>Status.NativeError</c> when the platform can't
    /// create the device.</description></item>
    /// </list>
    /// </remarks>
    public static GpuDeviceHandle Create(GpuDeviceOptions? options = null)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_gpu_device_options)
            : NativeGpuDeviceOptions(options, scope);
        MlnGpuDevice outDevice = default;
        Check(
            NativeMethods.mln_gpu_device_create(
                options is null ? null : &nativeOptions,
                &outDevice,
                Diagnostic
            )
        );
        scope.Accept();
        var owner = GpuDeviceHandle.Adopt(outDevice);
        return owner;
    }

    /// <summary>
    /// Reads what the device can do.
    /// </summary>
    /// <remarks>
    /// <para>Check <c>export_kinds</c> before you attach an exported ring, to
    /// choose between the ring and a way of your own to show the map.</para>
    /// <para>See <c>mln_gpu_device_get_capabilities</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// </remarks>
    public GpuDeviceCapabilities GetCapabilities()
    {
        using var call = Enter(this);
        var outCapabilities = new mln_gpu_device_capabilities
        {
            size = (uint)sizeof(mln_gpu_device_capabilities),
        };
        Check(NativeMethods.mln_gpu_device_get_capabilities(Handle, &outCapabilities, Diagnostic));
        return CopyGpuDeviceCapabilities(outCapabilities);
    }

    /// <summary>
    /// Gets the Vulkan instance, device, and graphics queue of a Vulkan device.
    /// </summary>
    /// <remarks>
    /// <para>Use them to make the images of a borrowed ring, or a surface, that
    /// sessions on the device render to. The sessions on the device share its
    /// graphics queue. To submit to the queue yourself, create the device with
    /// <c>GpuDeviceOptions.QueueLock</c>.</para>
    /// <para>See <c>mln_gpu_device_get_vulkan_context</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the device doesn't
    /// render with Vulkan.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public GpuDeviceVulkanContextResult GetVulkanContext()
    {
        using var call = Enter(this);
        var outResult = new mln_gpu_device_vulkan_context_result
        {
            size = (uint)sizeof(mln_gpu_device_vulkan_context_result),
        };
        Check(NativeMethods.mln_gpu_device_get_vulkan_context(Handle, &outResult, Diagnostic));
        return CopyGpuDeviceVulkanContextResult(outResult);
    }

    /// <summary>
    /// Releases a GPU device.
    /// </summary>
    /// <remarks>
    /// <para>Render sessions on the device keep using it until they detach, or
    /// their release finishes.</para>
    /// <para>See <c>mln_gpu_device_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Release()
    {
        state.Close();
    }

    /// <summary>
    /// Wraps your Metal device and command queue, so that sessions render on
    /// them.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_gpu_device_wrap_metal</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Metal</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public static GpuDeviceHandle WrapMetal(MetalDeviceDescriptor descriptor)
    {
        using var call = Enter(null);
        var nativeDescriptor = NativeMetalDeviceDescriptor(descriptor);
        MlnGpuDevice outDevice = default;
        Check(NativeMethods.mln_gpu_device_wrap_metal(&nativeDescriptor, &outDevice, Diagnostic));
        return GpuDeviceHandle.Adopt(outDevice);
    }

    /// <summary>
    /// Makes an OpenGL device whose share group is your context's.
    /// </summary>
    /// <remarks>
    /// <para>Each session on the device makes a context of its own that shares
    /// objects with yours, so that it can render into your textures and present
    /// to your surfaces. A device that the library creates is a share group of
    /// its own.</para>
    /// <para>WebGL has no share groups: a WebGL device is your context. Its
    /// sessions present to a web canvas target whose canvas is the context's
    /// own, and take turns in the context. The library resets the context's
    /// state that it tracks at the start of each frame, and leaves it undefined
    /// after.</para>
    /// <para>See <c>mln_gpu_device_wrap_open_gl</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.OpenGlSupportedContextProviderMask()</c> lacks the
    /// context's platform.</description></item>
    /// <item><description><c>Status.NativeError</c> when the platform rejects
    /// the context.</description></item>
    /// </list>
    /// </remarks>
    public static GpuDeviceHandle WrapOpenGl(OpenGlContextDescriptor context)
    {
        using var call = Enter(null);
        var nativeContext = NativeOpenGlContextDescriptor(context);
        MlnGpuDevice outDevice = default;
        Check(NativeMethods.mln_gpu_device_wrap_open_gl(&nativeContext, &outDevice, Diagnostic));
        return GpuDeviceHandle.Adopt(outDevice);
    }

    /// <summary>
    /// Wraps your Vulkan device and queue, so that sessions render on them.
    /// </summary>
    /// <remarks>
    /// <para>Sessions on the device render into your images, present to your
    /// surfaces, and submit to your queue, under your queue lock when you give
    /// one. Sessions of several devices that wrap one queue never submit to it
    /// at once.</para>
    /// <para>See <c>mln_gpu_device_wrap_vulkan</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Vulkan</c>.</description></item>
    /// <item><description><c>Status.NativeError</c> when the library can't load
    /// the device's functions.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public static GpuDeviceHandle WrapVulkan(VulkanDeviceDescriptor descriptor)
    {
        using var scope = new NativeCallScope(null);
        var nativeDescriptor = NativeVulkanDeviceDescriptor(descriptor, scope);
        MlnGpuDevice outDevice = default;
        Check(NativeMethods.mln_gpu_device_wrap_vulkan(&nativeDescriptor, &outDevice, Diagnostic));
        scope.Accept();
        var owner = GpuDeviceHandle.Adopt(outDevice);
        return owner;
    }
}
