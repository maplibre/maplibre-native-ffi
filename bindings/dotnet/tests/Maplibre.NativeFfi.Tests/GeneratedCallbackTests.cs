using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
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
        Assert.Equal(new CanonicalTileId(1, 2, 3), fetched);
        native.release_user_data(native.user_data);
        scope.Accept();
    }

    // A registration that native accepted is reachable only through its user data, so it must
    // survive a collection to run.
    [Fact]
    public void AnAcceptedRegistrationSurvivesACollection()
    {
        var calls = 0;
        var native = Prepare(() => Interlocked.Increment(ref calls));
        Gc.Collect();

        native.fetch_tile(native.user_data, default);
        native.release_user_data(native.user_data);
        Assert.Equal(1, calls);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static mln_custom_geometry_source_options Prepare(Action onFetch)
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => onFetch() },
            scope
        );
        scope.Accept();
        return native;
    }
}
