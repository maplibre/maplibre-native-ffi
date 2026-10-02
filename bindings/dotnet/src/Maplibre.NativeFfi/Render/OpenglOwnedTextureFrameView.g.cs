// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public sealed class OpenglOwnedTextureFrameView
{
    private readonly OpenglOwnedTextureFrame value;
    private readonly NativeViewScope scope;

    internal OpenglOwnedTextureFrameView(OpenglOwnedTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public ulong Generation => scope.Active(value).Generation;
    public uint Width => scope.Active(value).Width;
    public uint Height => scope.Active(value).Height;
    public double ScaleFactor => scope.Active(value).ScaleFactor;
    public ulong FrameId => scope.Active(value).FrameId;
    public uint Texture => scope.Active(value).Texture;
    public uint Target => scope.Active(value).Target;
    public uint InternalFormat => scope.Active(value).InternalFormat;
    public uint Format => scope.Active(value).Format;
    public uint Type => scope.Active(value).Type;
}
