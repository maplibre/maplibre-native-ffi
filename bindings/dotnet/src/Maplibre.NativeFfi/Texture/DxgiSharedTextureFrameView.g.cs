// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class DxgiSharedTextureFrameView
{
    private readonly DxgiSharedTextureFrame value;
    private readonly NativeViewScope scope;

    internal DxgiSharedTextureFrameView(DxgiSharedTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public uint Format => value.Format;
    public NativePointer Handle => scope.Active(value).Handle;
}
