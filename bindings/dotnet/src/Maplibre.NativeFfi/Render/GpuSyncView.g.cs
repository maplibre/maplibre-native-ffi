// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public sealed class GpuSyncView
{
    private readonly GpuSync value;
    private readonly NativeViewScope scope;

    internal GpuSyncView(GpuSync value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public GpuSyncKind Kind => scope.Active(value).Kind;
    public ulong Object => scope.Active(value).Object;
    public ulong Value => scope.Active(value).Value;
}
