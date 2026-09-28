// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Render;

public sealed class VulkanOwnedTextureFrameView
{
    private readonly VulkanOwnedTextureFrame value;
    private readonly NativeViewScope scope;

    internal VulkanOwnedTextureFrameView(VulkanOwnedTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return value.Generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return value.Width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return value.Height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return value.ScaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return value.FrameId;
        }
    }
    public ulong Image
    {
        get
        {
            scope.EnsureActive();
            return value.Image;
        }
    }
    public ulong ImageView
    {
        get
        {
            scope.EnsureActive();
            return value.ImageView;
        }
    }
    public NativePointer Device
    {
        get
        {
            scope.EnsureActive();
            return value.Device;
        }
    }
    public uint Format
    {
        get
        {
            scope.EnsureActive();
            return value.Format;
        }
    }
    public uint Layout
    {
        get
        {
            scope.EnsureActive();
            return value.Layout;
        }
    }
}
