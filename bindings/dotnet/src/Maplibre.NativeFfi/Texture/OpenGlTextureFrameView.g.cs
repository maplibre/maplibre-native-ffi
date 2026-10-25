// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class OpenGlTextureFrameView
{
    private readonly OpenGlTextureFrame value;
    private readonly NativeViewScope scope;

    internal OpenGlTextureFrameView(OpenGlTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public ulong Generation => value.Generation;
    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public ulong FrameId => value.FrameId;
    public uint Slot => value.Slot;
    public uint Texture => scope.Active(value).Texture;
    public uint Target => value.Target;
    public uint InternalFormat => value.InternalFormat;
    public uint Format => value.Format;
    public uint Type => value.Type;
}
