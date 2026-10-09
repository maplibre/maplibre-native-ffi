using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class GeneratedCallbackTests
{
    [Fact]
    public void GeneratedCallbacksSnapshotDelegatesCopyArgumentsAndContainFailures()
    {
        CanonicalTileId? fetched = null;
        var options = new CustomGeometrySourceOptions
        {
            FetchTile = tile => fetched = tile,
            CancelTile = _ => throw new InvalidOperationException("callback failure"),
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(options, scope);
        options.FetchTile = _ => throw new InvalidOperationException("mutated descriptor");
        var tile = new mln_canonical_tile_id
        {
            z = 1,
            x = 2,
            y = 3,
        };
        native.fetch_tile(native.user_data, tile);
        native.cancel_tile(native.user_data, tile);
        tile.x = 99;
        Assert.Equal(new CanonicalTileId(1, 2, 3), fetched);
        native.release_user_data(native.user_data);
        scope.Accept();
    }

    [Fact]
    public void RegistrationRootsFollowAcceptanceAndNativeRelease()
    {
        var accepted = Prepare(accept: true);
        var rejected = Prepare(accept: false);
        Gc.Collect();
        Assert.True(accepted.Root.IsAlive);
        Assert.False(rejected.Root.IsAlive);

        accepted.Native.release_user_data(accepted.Native.user_data);
        Gc.Collect();
        Assert.False(accepted.Root.IsAlive);
    }

    [Fact]
    public void RetiringAnOwnerRetainsCallbacksUntilNativeReleaseAndContainsStaleDispatch()
    {
        var owner = new NativeCallbackOwner();
        var first = PrepareOwned(owner);
        owner.Retire();
        var late = PrepareOwned(owner);
        Gc.Collect();
        Assert.True(first.Root.IsAlive);
        Assert.True(late.Root.IsAlive);
        first.Native.fetch_tile(first.Native.user_data, default);
        first.Native.release_user_data(first.Native.user_data);
        late.Native.release_user_data(late.Native.user_data);
        Gc.Collect();
        Assert.False(first.Root.IsAlive);
        Assert.False(late.Root.IsAlive);
        first.Native.fetch_tile(first.Native.user_data, default);
        first.Native.release_user_data(first.Native.user_data);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (mln_custom_geometry_source_options Native, WeakReference Root) PrepareOwned(
        NativeCallbackOwner owner
    )
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => GC.KeepAlive(owner) },
            scope
        );
        var weak = new WeakReference(NativeCallbackRoot.Value(native.user_data));
        scope.Accept(owner);
        return (native, weak);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (mln_custom_geometry_source_options Native, WeakReference Root) Prepare(
        bool accept
    )
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => { } },
            scope
        );
        var weak = new WeakReference(NativeCallbackRoot.Value(native.user_data));
        if (accept)
            scope.Accept();
        return (native, weak);
    }
}
