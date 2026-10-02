// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
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

    public ulong Generation => scope.Active(value).Generation;
    public uint Width => scope.Active(value).Width;
    public uint Height => scope.Active(value).Height;
    public double ScaleFactor => scope.Active(value).ScaleFactor;
    public ulong FrameId => scope.Active(value).FrameId;
    public ulong Image => scope.Active(value).Image;
    public ulong ImageView => scope.Active(value).ImageView;
    public NativePointer Device => scope.Active(value).Device;
    public uint Format => scope.Active(value).Format;
    public uint Layout => scope.Active(value).Layout;
}
