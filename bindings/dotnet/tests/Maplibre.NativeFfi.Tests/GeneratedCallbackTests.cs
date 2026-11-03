using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class GeneratedCallbackTests
{
    // The callback borrows the map that it names, so it can't release it.
    [Fact]
    public void GeneratedCallbacksCopyArgumentsAndContainFailures()
    {
        (ulong Map, string SourceId, CanonicalTileId Tile)? fetched = null;
        Exception? release = null;
        var options = new CustomGeometrySourceOptions
        {
            FetchTile = (map, sourceId, tile) =>
            {
                fetched = (map.Id, sourceId, tile);
                release = Record.Exception(map.Dispose);
            },
            CancelTile = (_, _, _) => throw new InvalidOperationException("callback failure"),
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(options, scope);
        var tile = new mln_canonical_tile_id
        {
            z = 1,
            x = 2,
            y = 3,
        };
        fixed (byte* id = "custom"u8)
        {
            var sourceId = new mln_buffer_view { data = id, size = 6 };
            native.fetch_tile(native.user_data, new MlnMap(7), sourceId, tile);
            native.cancel_tile(native.user_data, new MlnMap(7), sourceId, tile);
        }
        Assert.Equal((7UL, "custom", new CanonicalTileId(1, 2, 3)), fetched);
        Assert.Equal(Status.InvalidState, Assert.IsType<MaplibreException>(release).Status);
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

        native.fetch_tile(native.user_data, new MlnMap(7), default, default);
        native.release_user_data(native.user_data);
        Assert.Equal(1, calls);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static mln_custom_geometry_source_options Prepare(Action onFetch)
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = (_, _, _) => onFetch() },
            scope
        );
        scope.Accept();
        return native;
    }
}
