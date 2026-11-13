// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class OpaqueFdFrameView
{
    private readonly OpaqueFdFrame value;
    private readonly NativeViewScope scope;

    internal OpaqueFdFrameView(OpaqueFdFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public int Fd => scope.Active(value).Fd;
    public ulong AllocationSize => value.AllocationSize;
    public bool Dedicated => value.Dedicated;
    public uint Format => value.Format;
    public int DepthFd => scope.Active(value).DepthFd;
    public ulong DepthAllocationSize => value.DepthAllocationSize;
    public bool DepthDedicated => value.DepthDedicated;
    public uint DepthFormat => value.DepthFormat;
}
