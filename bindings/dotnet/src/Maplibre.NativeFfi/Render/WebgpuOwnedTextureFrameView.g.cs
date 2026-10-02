// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public sealed class WebgpuOwnedTextureFrameView
{
    private readonly WebgpuOwnedTextureFrame value;
    private readonly NativeViewScope scope;

    internal WebgpuOwnedTextureFrameView(WebgpuOwnedTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public ulong Generation => scope.Active(value).Generation;
    public uint Width => scope.Active(value).Width;
    public uint Height => scope.Active(value).Height;
    public double ScaleFactor => scope.Active(value).ScaleFactor;
    public ulong FrameId => scope.Active(value).FrameId;
    public NativePointer Texture => scope.Active(value).Texture;
    public NativePointer TextureView => scope.Active(value).TextureView;
    public NativePointer Device => scope.Active(value).Device;
    public uint Format => scope.Active(value).Format;
}
