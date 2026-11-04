// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class CreateAMap
{
    private static void ReportFailure(MaplibreException error) { }

    // Attaches a render target, as the render target guides show.
    private static Task<RenderSessionHandle> AttachTarget(
        MapHandle map
    ) => throw new NotImplementedException();

    internal static async Task Run()
    {
        // #region runtime
        await using var runtime = RuntimeHandle.Create();
        // #endregion runtime

        // #region map
        await using var map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(800, 600, 2),
                MapMode = MapMode.Continuous,
            }
        );
        await map.Ready;
        // #endregion map

        // #region style
        map.Discarding.SetStyleUrl(
            "https://demotiles.maplibre.org/style.json"
        );
        // #endregion style

        var update = new CameraUpdate
        {
            Mode = CameraUpdateMode.Jump,
            Camera = new CameraOptions { Zoom = 3 },
        };

        // #region wait
        await map.UpdateCameraAsync(update);

        // Use the discarding variant to ignore the result.
        map.Discarding.UpdateCamera(update);
        // #endregion wait

        // #region errors
        try
        {
            await map.UpdateCameraAsync(update);
        }
        catch (MaplibreException error)
        {
            // Its message describes the failure.
            ReportFailure(error);
        }
        // #endregion errors

        // #region close
        await using var session = await AttachTarget(map);
        // Leaving the scope releases the session, the map, and the runtime, in
        // that order, and waits for each.
        // #endregion close
    }
}
