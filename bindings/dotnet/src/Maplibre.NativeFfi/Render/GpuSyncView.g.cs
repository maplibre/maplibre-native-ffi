// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

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

    public GpuSyncKind Kind
    {
        get
        {
            scope.EnsureActive();
            return value.Kind;
        }
    }
    public ulong Object
    {
        get
        {
            scope.EnsureActive();
            return value.Object;
        }
    }
    public ulong Value
    {
        get
        {
            scope.EnsureActive();
            return value.Value;
        }
    }
}
