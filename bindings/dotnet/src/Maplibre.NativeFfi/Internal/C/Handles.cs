namespace Maplibre.NativeFfi.Internal.C;

// The C API spells every handle as uint64_t. One single-field readonly struct per
// handle type keeps the kinds distinct at compile time and marshals identically to
// the bare integer. A handle value carries no ownership and is safe to copy.

/// <summary>A handle the C API issued.</summary>
internal interface IMlnHandle
{
    /// <summary>The issued handle value; zero is the null handle.</summary>
    ulong Value { get; }
}

internal readonly struct MlnBuffer(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnRuntime(ulong value) : IMlnHandle
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

internal readonly struct MlnGeoJsonSourceData(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnRenderSession(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnWakeSource(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnResourceRequest(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnOfflineRegionSnapshot(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnOfflineRegionList(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnStyleIdList(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnStyleStringList(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}
