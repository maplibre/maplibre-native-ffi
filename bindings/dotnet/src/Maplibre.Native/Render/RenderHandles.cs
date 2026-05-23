using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Query;

namespace Maplibre.Native.Render;

/// <summary>Owner-thread render session handle bound to a map.</summary>
public sealed unsafe class RenderSessionHandle : IDisposable
{
    private readonly MapHandle map;
    private readonly NativeHandleState<mln_render_session> state;

    private RenderSessionHandle(MapHandle map, mln_render_session* handle)
    {
        this.map = map ?? throw new ArgumentNullException(nameof(map));
        state = new NativeHandleState<mln_render_session>(
            handle,
            static handle => NativeMethods.mln_render_session_destroy(handle),
            nameof(RenderSessionHandle));
    }

    public static RenderSessionHandle AttachMetalSurface(MapHandle map, MetalSurfaceDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_metal_surface_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanSurface(MapHandle map, VulkanSurfaceDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_vulkan_surface_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachMetalOwnedTexture(MapHandle map, MetalOwnedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_metal_owned_texture_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachMetalBorrowedTexture(MapHandle map, MetalBorrowedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_metal_borrowed_texture_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanOwnedTexture(MapHandle map, VulkanOwnedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_vulkan_owned_texture_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanBorrowedTexture(MapHandle map, VulkanBorrowedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_vulkan_borrowed_texture_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    internal mln_render_session* Pointer => state.Pointer;

    public bool IsClosed => state.IsClosed;

    public void Resize(uint width, uint height, double scaleFactor)
    {
        NativeStatus.Check(NativeMethods.mln_render_session_resize(Pointer, width, height, scaleFactor));
    }

    public void RenderUpdate()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_render_update(Pointer));
    }

    public void Detach()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_detach(Pointer));
    }

    public void ReduceMemoryUse()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_reduce_memory_use(Pointer));
    }

    public void ClearData()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_clear_data(Pointer));
    }

    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_dump_debug_logs(Pointer));
    }

    public void SetFeatureState(FeatureStateSelector selector, JsonValue value)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        using var nativeValue = NativeJsonValue.From(value);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(NativeMethods.mln_render_session_set_feature_state(Pointer, &selectorValue, nativeValue.Pointer));
    }

    public JsonValue GetFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        mln_json_snapshot* snapshot = null;
        NativeStatus.Check(NativeMethods.mln_render_session_get_feature_state(Pointer, &selectorValue, &snapshot));
        return ValueStructs.ReadJsonSnapshot(snapshot) ?? new JsonValue.Object([]);
    }

    public void RemoveFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(NativeMethods.mln_render_session_remove_feature_state(Pointer, &selectorValue));
    }

    public TextureImageInfo TextureImageInfo()
    {
        var info = NativeMethods.mln_texture_image_info_default();
        var status = NativeMethods.mln_texture_read_premultiplied_rgba8(Pointer, null, 0, &info);
        var copied = RenderStructs.FromNative(info);
        if (status == mln_status.MLN_STATUS_OK || (status == mln_status.MLN_STATUS_INVALID_ARGUMENT && copied.ByteLength > 0))
        {
            return copied;
        }

        NativeStatus.Check(status);
        throw new InvalidOperationException("Unreachable native texture status.");
    }

    public TextureImageInfo ReadPremultipliedRgba8(NativeBuffer buffer)
    {
        ArgumentNullException.ThrowIfNull(buffer);
        var info = NativeMethods.mln_texture_image_info_default();
        fixed (byte* data = buffer.Span)
        {
            NativeStatus.Check(NativeMethods.mln_texture_read_premultiplied_rgba8(Pointer, buffer.ByteLength == 0 ? null : data, buffer.ByteLength, &info));
        }
        return RenderStructs.FromNative(info);
    }

    public PremultipliedRgba8Image ReadPremultipliedRgba8()
    {
        var info = TextureImageInfo();
        using var buffer = new NativeBuffer((nuint)info.ByteLength);
        var readInfo = ReadPremultipliedRgba8(buffer);
        return new PremultipliedRgba8Image(buffer.Span.ToArray(), readInfo);
    }

    /// <summary>Destroys the render session on the map owner thread.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
        GC.KeepAlive(map);
    }
}

public sealed class MetalOwnedTextureFrameHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public MetalOwnedTextureFrame Frame => IsClosed ? throw new ObjectDisposedException(nameof(MetalOwnedTextureFrameHandle)) : default;
    public void Dispose() => IsClosed = true;
}

public sealed class VulkanOwnedTextureFrameHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public VulkanOwnedTextureFrame Frame => IsClosed ? throw new ObjectDisposedException(nameof(VulkanOwnedTextureFrameHandle)) : default;
    public void Dispose() => IsClosed = true;
}

public sealed class FrameScope : IDisposable
{
    public bool IsClosed { get; private set; }
    public void Dispose() => IsClosed = true;
}
