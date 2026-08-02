namespace Maplibre.NativeFfi.Internal.C;

// The C API spells every handle as the same uint64_t, so a generated `ulong`
// would let a map be passed where a runtime is expected. One readonly struct per
// handle type keeps the kinds distinct at compile time. Each is a single ulong
// field, so it passes in a register exactly as the bare integer would.
//
// The value is the handle the C API issued. It names one object for the life of
// the process, carries no ownership, and is safe to copy, compare, and hash.

/// <summary>A handle the C API issued.</summary>
internal interface IMlnHandle
{
    /// <summary>The issued handle value; zero is the null handle.</summary>
    ulong Value { get; }
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

internal readonly struct MlnJsonSnapshot(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnStyleIdList(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnFeatureQueryResult(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}

internal readonly struct MlnFeatureExtensionResult(ulong value) : IMlnHandle
{
    public ulong Value { get; } = value;

    public bool IsNull => Value == 0;
}
