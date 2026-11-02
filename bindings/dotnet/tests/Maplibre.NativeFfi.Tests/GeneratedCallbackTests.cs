using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class GeneratedCallbackTests
{
    [Fact]
    public void GeneratedCallbacksCopyArgumentsAndContainFailures()
    {
        CanonicalTileId? fetched = null;
        var options = new CustomGeometrySourceOptions
        {
            FetchTile = tile => fetched = tile,
            CancelTile = _ => throw new InvalidOperationException("callback failure"),
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(options, scope);
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

    // A registration that native accepted is reachable only through its user data, so it must
    // survive a collection to run, including when the owning handle failed to construct.
    [Fact]
    public void AcceptedRegistrationsSurviveACollection()
    {
        var calls = 0;
        var owned = Prepare(() => Interlocked.Increment(ref calls), adoptionFails: false);
        var orphaned = Prepare(() => Interlocked.Increment(ref calls), adoptionFails: true);
        Gc.Collect();

        owned.fetch_tile(owned.user_data, default);
        orphaned.fetch_tile(orphaned.user_data, default);
        owned.release_user_data(owned.user_data);
        orphaned.release_user_data(orphaned.user_data);
        Assert.Equal(2, calls);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static mln_custom_geometry_source_options Prepare(Action onFetch, bool adoptionFails)
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => onFetch() },
            scope
        );
        // A generated attachment accepts its registrations once native admits the call, before it
        // adopts the handle, so a failed adoption leaves them to native.
        scope.Accept();
        if (adoptionFails)
        {
            var failure = new FormatException("Owner construction failed.");
            Assert.Same(
                failure,
                Assert.Throws<FormatException>(() =>
                    NativeHandleState<MlnRuntime>.Adopt<RuntimeHandle>(
                        SyntheticHandles.Runtime(1234),
                        () => throw failure,
                        (_, _) => mln_status.MLN_STATUS_OK
                    )
                )
            );
        }
        return native;
    }
}
