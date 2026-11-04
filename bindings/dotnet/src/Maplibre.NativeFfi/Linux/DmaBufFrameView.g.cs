// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class DmaBufFrameView
{
    private readonly DmaBufFrame value;
    private readonly NativeViewScope scope;

    internal DmaBufFrameView(DmaBufFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public uint Fourcc => value.Fourcc;
    public ulong Modifier => value.Modifier;
    public ImmutableArray<DmaBufPlane> Planes => scope.Active(value).Planes;
}
