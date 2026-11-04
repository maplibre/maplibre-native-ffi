// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A Vulkan instance, device, and queue that the library creates and owns.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_device</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class VulkanDeviceHandle : IDisposable, INativeOwner<MlnVulkanDevice>
{
    private readonly NativeHandleState<MlnVulkanDevice> state;

    internal VulkanDeviceHandle(MlnVulkanDevice handle)
    {
        state = new(handle, Abandon, nameof(VulkanDeviceHandle), Abandon);
    }

    internal static VulkanDeviceHandle Adopt(MlnVulkanDevice handle) =>
        NativeHandleState<MlnVulkanDevice>.Adopt(
            handle,
            () => new VulkanDeviceHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnVulkanDevice live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_vulkan_device_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnVulkanDevice> INativeOwner<MlnVulkanDevice>.State => state;
    internal MlnVulkanDevice Handle => state.Handle;

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
    /// Creates a Vulkan instance, device, and graphics queue for render
    /// sessions.
    /// </summary>
    /// <remarks>
    /// <para>Attach sessions with the context from
    /// <c>VulkanDeviceHandle.GetContext()</c>, in place of a device of your
    /// own. Without <c>device_uuid</c> or <c>device_luid</c>, the device uses a
    /// discrete GPU over an integrated one, and an integrated GPU over any
    /// other. The device enables the features that rendering needs, and the
    /// external memory and semaphore extensions that the GPU offers.</para>
    /// <para>See <c>mln_vulkan_device_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Vulkan</c>, or when <c>presentation</c> is true and
    /// the Vulkan loader or the GPU can't present.</description></item>
    /// <item><description><c>Status.NotFound</c> when no GPU matches
    /// <c>device_uuid</c> and <c>device_luid</c>.</description></item>
    /// <item><description><c>Status.NativeError</c> when there is no Vulkan
    /// loader, no GPU has a graphics queue, or Vulkan can't create the instance
    /// or the device.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public static VulkanDeviceHandle Create(VulkanDeviceOptions? options = null)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_vulkan_device_options)
            : NativeVulkanDeviceOptions(options, scope);
        MlnVulkanDevice outDevice = default;
        Check(
            NativeMethods.mln_vulkan_device_create(
                options is null ? null : &nativeOptions,
                &outDevice,
                Diagnostic
            )
        );
        scope.Accept();
        return VulkanDeviceHandle.Adopt(outDevice);
    }

    /// <summary>
    /// Gets the context to attach render sessions to the device with.
    /// </summary>
    /// <remarks>
    /// <para>Pass it as the context of <c>MapHandle.AttachVulkanSurface()</c>
    /// or <c>MapHandle.AttachVulkanBorrowedTexture()</c>. The sessions on the
    /// device share its one graphics queue, and the library orders their
    /// submissions to it. To submit to the queue yourself, set
    /// <c>RenderSessionAttachOptions.QueueLock</c> on each session.</para>
    /// <para>See <c>mln_vulkan_device_get_context</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html">C API reference</see>.</para>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public VulkanDeviceContextResult GetContext()
    {
        using var call = Enter(this);
        var outResult = new mln_vulkan_device_context_result
        {
            size = (uint)sizeof(mln_vulkan_device_context_result),
        };
        Check(NativeMethods.mln_vulkan_device_get_context(Handle, &outResult, Diagnostic));
        return CopyVulkanDeviceContextResult(outResult);
    }

    /// <summary>
    /// Releases a Vulkan device.
    /// </summary>
    /// <remarks>
    /// <para>Render sessions that attached with the device's context keep using
    /// it until they detach or retire, and the library destroys the device
    /// after the last of them.</para>
    /// <para>See <c>mln_vulkan_device_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Release()
    {
        state.Close();
    }
}
