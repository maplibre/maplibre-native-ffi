// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi.Internal.C;

internal interface IMlnHandle
{
    ulong Value { get; }
}

internal readonly struct MlnAcquiredFrame(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnAdapterLogQueue(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnAdapterResourceRequestQueue(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnBuffer(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnEventBatch(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnGeoJsonSourceData(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnMap(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnMapProjection(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnRenderFrameBatch(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnRenderSession(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnResourceRequest(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}

internal readonly struct MlnRuntime(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;
    public bool IsNull => Value == 0;
}
