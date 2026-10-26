using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>One representative of each generated value shape, crossing to native and back.</summary>
public sealed class GeneratedValueTests
{
    // JSON goes in and comes back as a string, transcoded through UTF-8.
    [Fact]
    public void CallerSizedTextReturnsResultsLongerThanItsFirstStorage()
    {
        using var zoom = StyleExpressionHandle.Create(
            """["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]"""
        );
        Assert.Equal("50.0", zoom.Evaluate(new StyleExpressionInputs { Zoom = 5 }));

        // The result outgrows the call's first storage, so the call runs again, and its
        // multibyte characters decode whole.
        var text = $"\"{string.Concat(Enumerable.Repeat("aé", 500))}\"";
        using var expression = StyleExpressionHandle.Create(text);
        Assert.Equal(text, expression.Evaluate());
    }

    // C borrows a byte input only for the call, so the binding pins the caller's span instead of
    // copying it. An empty span pins to null, which C accepts with a zero size. No source has the
    // ID, so each accepted call completes with NotFound.
    [Fact]
    public async Task ByteInputsCrossAsPinnedSpans()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();
        var tile = new CanonicalTileId(0, 0, 0);
        var filled = await Assert.ThrowsAsync<MaplibreException>(() =>
            fixture.Map.SetCustomVectorSourceTileDataAsync(
                "missing",
                tile,
                StyleVectorTileEncoding.Mvt,
                [1, 2, 3]
            )
        );
        Assert.Equal(Status.NotFound, filled.Status);
        var empty = await Assert.ThrowsAsync<MaplibreException>(() =>
            fixture.Map.SetCustomVectorSourceTileDataAsync(
                "missing",
                tile,
                StyleVectorTileEncoding.Mvt,
                ReadOnlySpan<byte>.Empty
            )
        );
        Assert.Equal(Status.NotFound, empty.Status);
    }

    [Fact]
    public async Task SynchronousCallsFillCallerSizedOutputsInInputOrder()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        using var projection = MapProjectionHandle.Create(fixture.Map.GetSnapshot().View);
        LatLng[] coordinates = [new(0, 0), new(10, 20), new(-12, -30)];
        var points = projection.PixelsForLatLngs(coordinates);
        Assert.Equal(coordinates.Select(projection.PixelForLatLng), points);
        Assert.Equal(points.Select(projection.LatLngForPixel), projection.LatLngsForPixels(points));
        Assert.Equal(
            points.Select(projection.LatLngForPixelUnwrapped),
            projection.LatLngsForPixelsUnwrapped(points)
        );
        Assert.Empty(projection.PixelsForLatLngs([]));
        Assert.Empty(projection.LatLngsForPixels([]));
        Assert.Empty(projection.LatLngsForPixelsUnwrapped([]));
    }

    private const string EmptyGeoJsonSource =
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}""";

    [Fact]
    public async Task HostStringsUseViewsAndNativeLentStringsDecode()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        // A source ID crosses as an explicit-length view, so an embedded NUL is part of the ID.
        await fixture.Map.AddStyleSourceJsonAsync("before\0after", EmptyGeoJsonSource);
        Assert.NotNull(await fixture.Map.GetStyleSourceAsync("before\0after"));
        Assert.Null(await fixture.Map.GetStyleSourceAsync("before"));

        // The layer ID comes back as text that native lends for the completion.
        await fixture.Map.AddStyleLayerJsonAsync("""{"id":"café-层","type":"background"}""", "");
        Assert.Equal(["café-层"], (await fixture.Map.ListStyleLayersAsync()).Select(l => l.Id));

        // Native rejects a NUL in a URL view before a C-string consumer reads it.
        var error = Assert.Throws<MaplibreException>(() =>
        {
            _ = fixture.Map.SetStyleUrlAsync("style\0.json");
        });
        Assert.Contains("embedded NUL", error.Diagnostic, StringComparison.Ordinal);
        Assert.Equal(Status.InvalidArgument, error.Status);
    }

    [Fact]
    public async Task PresenceFieldsSendOnlyTheValuesTheCallerSet()
    {
        await using var fixture = await NativeFixture.CreateAsync();

        await fixture.Map.UpdateCameraAsync(
            new CameraUpdate
            {
                Camera = new CameraOptions { Center = new LatLng(10, 20), Zoom = 3 },
            }
        );
        // Only the bearing is present, so the center and zoom stay as they were.
        await fixture.Map.UpdateCameraAsync(
            new CameraUpdate { Camera = new CameraOptions { Bearing = 30 } }
        );
        var camera = (await fixture.Map.GetCameraAsync()).Camera;

        Assert.Equal(10, camera.Center.Latitude, 6);
        Assert.Equal(20, camera.Center.Longitude, 6);
        Assert.Equal(3, camera.Zoom, 6);
        Assert.Equal(30, camera.Bearing, 6);
    }

    // A record struct's own default leaves a string or a nested record null, so its parameterless
    // constructor sets them to their zero values.
    [Fact]
    public void AParameterlessConstructorSetsEachMemberToItsZeroValue()
    {
        Assert.Equal(new HttpHeaderRule(default, default, "", []), new HttpHeaderRule());
        Assert.Equal(new CameraOptions(), new CameraUpdate().Camera);
        Assert.Equal("", new HttpHeader { Name = "Accept" }.Value);
    }

    [Fact]
    public unsafe void AnInputUnionWritesTheArmItHolds()
    {
        using var scope = new NativeCallScope();

        var box = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.Box(
                new ScreenBox(new ScreenPoint(3, 4), new ScreenPoint(5, 6))
            ),
            scope
        );
        var line = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.LineString(
                new ScreenLineString([new ScreenPoint(7, 8), new ScreenPoint(9, 10)])
            ),
            scope
        );

        Assert.Equal(
            mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
            box.type
        );
        Assert.Equal(6, box.data.box.max.y);
        Assert.Equal(
            mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
            line.type
        );
        Assert.Equal(2u, line.data.line_string.point_count);
        Assert.Equal(9, line.data.line_string.points[1].x);
    }

    [Fact]
    public unsafe void AnOutputUnionWithAnUnknownTagKeepsTheTag()
    {
        var copied = GeneratedValues.CopyOfflineRegionDefinition(
            new mln_offline_region_definition
            {
                size = (uint)sizeof(mln_offline_region_definition),
                type = (mln_offline_region_definition_type)999,
            }
        );

        var unknown = Assert.IsType<OfflineRegionDefinition.Unknown>(copied);
        Assert.Equal(999u, unknown.Tag);
    }
}
