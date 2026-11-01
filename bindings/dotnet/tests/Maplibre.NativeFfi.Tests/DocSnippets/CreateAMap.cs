// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class CreateAMap
{
    // Attaches a render target, as the Rendering guides show.
    private static Task<RenderSessionHandle> AttachTarget(MapHandle map) =>
        throw new NotImplementedException();

    internal static async Task Run()
    {
        // #region runtime
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        // #endregion runtime

        // #region map
        var map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(800, 600, 2),
                MapMode = MapMode.Continuous,
            }
        );
        await map.Ready;
        // #endregion map

        var session = await AttachTarget(map);

        // #region close
        session.Dispose();
        await map.CloseAsync();
        await runtime.CloseAsync();
        // #endregion close
    }
}
