// Snippets for the docs site's Query page. They compile with the tests, which
// never run them.

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Query
{
    private static void UseFeature(string feature) { }

    internal static async Task Run(RenderSessionHandle session, ScreenPoint point)
    {
        // #region rendered
        var box = new ScreenBox(
            new ScreenPoint(point.X - 5, point.Y - 5),
            new ScreenPoint(point.X + 5, point.Y + 5)
        );
        var hits = await session.QueryRenderedFeaturesAsync(
            new RenderedQueryGeometry.Box(box),
            new RenderedFeatureQueryOptions { LayerIds = ["poi"] }
        );
        foreach (var hit in hits)
        {
            // hit.Feature is GeoJSON text.
            UseFeature(hit.Feature);
        }
        // #endregion rendered

        // #region source
        var features = await session.QuerySourceFeaturesAsync(
            "openmaptiles",
            new SourceFeatureQueryOptions
            {
                SourceLayerIds = ["transportation"],
                // A filter from the MapLibre style spec, as JSON.
                Filter = """["==", ["get", "class"], "motorway"]""",
            }
        );
        foreach (var feature in features)
        {
            UseFeature(feature.Feature);
        }
        // #endregion source
    }
}
