// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class MetalTextureFrameView
{
    private readonly MetalTextureFrame value;
    private readonly NativeViewScope scope;

    internal MetalTextureFrameView(MetalTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public NativePointer Texture => scope.Active(value).Texture;
    public NativePointer Device => scope.Active(value).Device;
    public ulong PixelFormat => value.PixelFormat;
}
