// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class IoSurfaceFrameView
{
    private readonly IoSurfaceFrame value;
    private readonly NativeViewScope scope;

    internal IoSurfaceFrameView(IoSurfaceFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public NativePointer Surface => scope.Active(value).Surface;
}
