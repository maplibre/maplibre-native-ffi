// Snippets for the docs site's Style page. They compile with the tests, which
// never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Style
{
    private static PremultipliedRgba8Image DrawImage(string imageId) =>
        new(1, 1, 4, [255, 0, 0, 255]);

    // pixels holds 16 x 16 premultiplied RGBA pixels.
    internal static void Run(MapHandle map, ImmutableArray<byte> pixels)
    {
        // #region load
        map.Discarding.SetStyleUrl("https://demotiles.maplibre.org/style.json");
        // #endregion load

        // #region add
        map.Discarding.AddStyleSourceJson(
            "satellite",
            """
            {"type":"raster","tileSize":256,"tiles":["https://tiles.example.com/{z}/{x}/{y}.png"]}
            """
        );

        // Draw the layer under the "countries-label" layer.
        map.Discarding.AddStyleLayerJson(
            """{"id":"satellite","type":"raster","source":"satellite"}""",
            "countries-label"
        );
        // #endregion add

        // #region geojson
        var data = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[-122.4,37.8]}"""
        );
        map.Discarding.AddGeoJsonSourceData("points", data);
        // The source keeps its own reference to the data.
        data.Dispose();

        // Later, prepare new data with the same options, and set it.
        var next = GeoJsonSourceDataHandle.Create(
            """{"type":"Point","coordinates":[2.35,48.86]}"""
        );
        map.Discarding.SetGeoJsonSourceData("points", next);
        next.Dispose();
        // #endregion geojson

        // #region set-property
        // Show both changes in the same frame.
        map.Discarding.BeginCommandGroup();
        map.Discarding.SetStyleLayerProperty("satellite", "visibility", "\"none\"");
        map.Discarding.SetStyleLayerProperty(
            "countries-boundary",
            "line-color",
            """["interpolate",["linear"],["zoom"],5,"#9cf",12,"#06c"]"""
        );
        map.Discarding.EndCommandGroup();
        // #endregion set-property

        // #region image
        map.Discarding.SetStyleImage("marker", new PremultipliedRgba8Image(16, 16, 16 * 4, pixels));

        // An empty layer ID draws the layer on top.
        map.Discarding.AddStyleLayerJson(
            """
            {"id":"markers","type":"symbol","source":"points","layout":{"icon-image":"marker"}}
            """,
            ""
        );
        // #endregion image
    }

    internal static void RunEvents(RuntimeHandle runtime, MapHandle map)
    {
        // #region missing-image
        // Runs on a library thread.
        runtime.Discarding.SetEventHandler(
            new RuntimeEventHandler(events =>
            {
                foreach (var e in events)
                {
                    if (e.Source != map.Id || e.Type != RuntimeEventType.MapStyleImageMissing)
                        continue;

                    // The message holds the image's ID.
                    map.Discarding.SetStyleImage(e.Message, DrawImage(e.Message));
                }
            })
        );
        // #endregion missing-image
    }
}
