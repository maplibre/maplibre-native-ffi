using Maplibre.Native.Internal.C;
using Maplibre.Native.Query;
using Maplibre.Native.Render;

namespace Maplibre.Native.Internal.Struct;

internal static unsafe class RenderStructs
{
    internal static mln_render_target_extent ToNative(RenderTargetExtent extent) => new()
    {
        size = (uint)sizeof(mln_render_target_extent),
        width = extent.Width,
        height = extent.Height,
        scale_factor = extent.ScaleFactor,
    };

    internal static mln_metal_context_descriptor ToNative(MetalContextDescriptor? context) => new()
    {
        size = (uint)sizeof(mln_metal_context_descriptor),
        device = context is null ? null : (void*)context.Device.Address,
    };

    internal static mln_vulkan_context_descriptor ToNative(VulkanContextDescriptor? context) => new()
    {
        size = (uint)sizeof(mln_vulkan_context_descriptor),
        instance = context is null ? null : (void*)context.Instance.Address,
        physical_device = context is null ? null : (void*)context.PhysicalDevice.Address,
        device = context is null ? null : (void*)context.Device.Address,
        graphics_queue = context is null ? null : (void*)context.Queue.Address,
        graphics_queue_family_index = context?.GraphicsQueueFamilyIndex ?? 0,
    };

    internal static mln_metal_surface_descriptor ToNative(MetalSurfaceDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_metal_surface_descriptor { size = (uint)sizeof(mln_metal_surface_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.context = ToNative(descriptor.Context);
        native.layer = (void*)descriptor.Layer.Address;
        return native;
    }

    internal static mln_vulkan_surface_descriptor ToNative(VulkanSurfaceDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_vulkan_surface_descriptor { size = (uint)sizeof(mln_vulkan_surface_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.context = ToNative(descriptor.Context);
        native.surface = (void*)descriptor.Surface.Address;
        return native;
    }

    internal static mln_metal_owned_texture_descriptor ToNative(MetalOwnedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_metal_owned_texture_descriptor { size = (uint)sizeof(mln_metal_owned_texture_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.context = ToNative(descriptor.Context);
        return native;
    }

    internal static mln_metal_borrowed_texture_descriptor ToNative(MetalBorrowedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_metal_borrowed_texture_descriptor { size = (uint)sizeof(mln_metal_borrowed_texture_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.texture = (void*)descriptor.Texture.Address;
        return native;
    }

    internal static mln_vulkan_owned_texture_descriptor ToNative(VulkanOwnedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_vulkan_owned_texture_descriptor { size = (uint)sizeof(mln_vulkan_owned_texture_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.context = ToNative(descriptor.Context);
        return native;
    }

    internal static mln_vulkan_borrowed_texture_descriptor ToNative(VulkanBorrowedTextureDescriptor descriptor)
    {
        ArgumentNullException.ThrowIfNull(descriptor);
        var native = new mln_vulkan_borrowed_texture_descriptor { size = (uint)sizeof(mln_vulkan_borrowed_texture_descriptor) };
        native.extent = ToNative(descriptor.Extent);
        native.context = ToNative(descriptor.Context);
        native.image = (void*)descriptor.Image.Address;
        native.image_view = (void*)descriptor.ImageView.Address;
        native.format = descriptor.Format;
        native.initial_layout = descriptor.InitialLayout;
        native.final_layout = descriptor.FinalLayout;
        return native;
    }

    internal static TextureImageInfo FromNative(mln_texture_image_info info) =>
        new(info.width, info.height, info.stride, (ulong)info.byte_length);

    internal static MetalOwnedTextureFrame FromNative(mln_metal_owned_texture_frame frame, FrameScope scope) =>
        new(
            scope,
            frame.generation,
            frame.width,
            frame.height,
            frame.scale_factor,
            frame.frame_id,
            new NativePointer((nint)frame.texture),
            new NativePointer((nint)frame.device),
            frame.pixel_format);

    internal static VulkanOwnedTextureFrame FromNative(mln_vulkan_owned_texture_frame frame, FrameScope scope) =>
        new(
            scope,
            frame.generation,
            frame.width,
            frame.height,
            frame.scale_factor,
            frame.frame_id,
            new NativePointer((nint)frame.image),
            new NativePointer((nint)frame.image_view),
            new NativePointer((nint)frame.device),
            frame.format,
            frame.layout);
}

internal sealed class NativeFeatureStateSelector : IDisposable
{
    private readonly NativeStringView sourceId;
    private readonly NativeStringView? sourceLayerId;
    private readonly NativeStringView? featureId;
    private readonly NativeStringView? stateKey;

    private NativeFeatureStateSelector(
        mln_feature_state_selector value,
        NativeStringView sourceId,
        NativeStringView? sourceLayerId,
        NativeStringView? featureId,
        NativeStringView? stateKey)
    {
        Value = value;
        this.sourceId = sourceId;
        this.sourceLayerId = sourceLayerId;
        this.featureId = featureId;
        this.stateKey = stateKey;
    }

    internal mln_feature_state_selector Value { get; }

    internal static NativeFeatureStateSelector From(FeatureStateSelector selector)
    {
        ArgumentNullException.ThrowIfNull(selector);
        var sourceId = NativeStringView.From(selector.SourceId, nameof(selector.SourceId));
        NativeStringView? sourceLayerId = null;
        NativeStringView? featureId = null;
        NativeStringView? stateKey = null;
        try
        {
            var value = new mln_feature_state_selector
            {
                size = (uint)System.Runtime.CompilerServices.Unsafe.SizeOf<mln_feature_state_selector>(),
                source_id = sourceId.Value,
            };
            if (selector.SourceLayerId is { } sourceLayer)
            {
                sourceLayerId = NativeStringView.From(sourceLayer, nameof(selector.SourceLayerId));
                value.fields |= (uint)mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
                value.source_layer_id = sourceLayerId.Value;
            }
            if (selector.FeatureId is { } feature)
            {
                featureId = NativeStringView.From(feature, nameof(selector.FeatureId));
                value.fields |= (uint)mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
                value.feature_id = featureId.Value;
            }
            if (selector.StateKey is { } state)
            {
                stateKey = NativeStringView.From(state, nameof(selector.StateKey));
                value.fields |= (uint)mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
                value.state_key = stateKey.Value;
            }

            return new NativeFeatureStateSelector(value, sourceId, sourceLayerId, featureId, stateKey);
        }
        catch
        {
            sourceId.Dispose();
            sourceLayerId?.Dispose();
            featureId?.Dispose();
            stateKey?.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        sourceId.Dispose();
        sourceLayerId?.Dispose();
        featureId?.Dispose();
        stateKey?.Dispose();
    }
}
