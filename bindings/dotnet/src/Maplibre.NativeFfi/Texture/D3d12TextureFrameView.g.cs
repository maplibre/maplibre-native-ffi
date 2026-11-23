// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class D3d12TextureFrameView
{
    private readonly D3d12TextureFrame value;
    private readonly NativeViewScope scope;

    internal D3d12TextureFrameView(D3d12TextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public uint Width => value.Width;
    public uint Height => value.Height;
    public double ScaleFactor => value.ScaleFactor;
    public uint Slot => value.Slot;
    public NativePointer Resource => scope.Active(value).Resource;
    public NativePointer Device => scope.Active(value).Device;
    public uint Format => value.Format;
    public uint State => value.State;
    public NativePointer DepthResource => scope.Active(value).DepthResource;
    public uint DepthFormat => value.DepthFormat;
    public uint DepthState => value.DepthState;
}
