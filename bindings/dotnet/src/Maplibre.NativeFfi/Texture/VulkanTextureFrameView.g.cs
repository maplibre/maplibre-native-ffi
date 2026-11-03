// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class VulkanTextureFrameView
{
    private readonly VulkanTextureFrame value;
    private readonly NativeViewScope scope;

    internal VulkanTextureFrameView(VulkanTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public ulong Image => scope.Active(value).Image;
    public ulong ImageView => scope.Active(value).ImageView;
    public NativePointer Device => scope.Active(value).Device;
    public uint Format => value.Format;
    public uint Layout => value.Layout;
}
