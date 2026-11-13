// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class AndroidHardwareBufferFrameView
{
    private readonly AndroidHardwareBufferFrame value;
    private readonly NativeViewScope scope;

    internal AndroidHardwareBufferFrameView(AndroidHardwareBufferFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public NativePointer Buffer => scope.Active(value).Buffer;
}
