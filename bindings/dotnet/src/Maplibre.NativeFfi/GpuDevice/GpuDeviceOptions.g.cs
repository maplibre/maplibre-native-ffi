// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>GpuDeviceHandle.Create()</c>.
/// </summary>
/// <remarks>
/// <para>Name the GPU by at most one of <c>adapter_index</c>,
/// <c>adapter_uuid</c>, and <c>adapter_luid</c>. Without one, the device
/// prefers a discrete GPU. An OpenGL device takes none of them: it renders on
/// the GPU that the platform picks.</para>
/// <para>See <c>mln_gpu_device_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record GpuDeviceOptions
{
    /// <summary>
    /// The backend that the device renders with. By default, the platform's
    /// own: Metal on Apple platforms, Vulkan elsewhere, or OpenGL where the
    /// system has no Vulkan driver, and in a browser the build's one backend:
    /// WebGPU or OpenGL (WebGL2).
    /// </summary>
    public RenderBackend? Backend { get; init; }

    /// <summary>
    /// The GPU to use, as an index into the backend's list of GPUs.
    /// </summary>
    public uint? AdapterIndex { get; init; }

    /// <summary>
    /// The 16-byte UUID of the GPU to use: Vulkan's
    /// <c>VkPhysicalDeviceIDProperties.deviceUUID</c>. Only a Vulkan device
    /// takes one.
    /// </summary>
    public ImmutableArray<byte>? AdapterUuid
    {
        get => AdapterUuidStorage?.AsImmutable();
        init => AdapterUuidStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<byte>? AdapterUuidStorage { get; init; }

    /// <summary>
    /// The 8-byte LUID of the GPU to use: on Windows, its DXGI adapter LUID,
    /// which Vulkan reports as <c>VkPhysicalDeviceIDProperties.deviceLUID</c>.
    /// Only a Vulkan device takes one.
    /// </summary>
    public ImmutableArray<byte>? AdapterLuid
    {
        get => AdapterLuidStorage?.AsImmutable();
        init => AdapterLuidStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<byte>? AdapterLuidStorage { get; init; }

    /// <summary>
    /// The OpenGL API that an OpenGL device renders with. An EGL device
    /// defaults to <c>OpenGlClientApi.Gles</c>, and takes
    /// <c>OpenGlClientApi.Gl</c> too on Linux and FreeBSD. A WGL device takes
    /// only <c>OpenGlClientApi.Gl</c>, its default, and a WebGL device only
    /// <c>OpenGlClientApi.Gles</c>, its default.
    /// </summary>
    public OpenGlClientApi? OpenGlClientApi { get; init; }

    /// <summary>
    /// Whether the sessions of an EGL device take turns in one OpenGL context
    /// instead of having one each.
    /// </summary>
    public bool? OpenGlSingleContext { get; init; }

    /// <summary>
    /// Your lock on a Vulkan device's queue, when you submit to the queue that
    /// <c>GpuDeviceHandle.GetVulkanContext()</c> reports. Only a Vulkan device
    /// takes one.
    /// </summary>
    public QueueLock QueueLock { get; init; }
}
