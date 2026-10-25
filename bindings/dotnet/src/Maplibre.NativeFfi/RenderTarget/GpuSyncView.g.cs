// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed class GpuSyncView
{
    private readonly GpuSync value;
    private readonly NativeViewScope scope;

    internal GpuSyncView(GpuSync value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public GpuSyncKind Kind => value.Kind;
    public ulong Handle => scope.Active(value).Handle;
}
