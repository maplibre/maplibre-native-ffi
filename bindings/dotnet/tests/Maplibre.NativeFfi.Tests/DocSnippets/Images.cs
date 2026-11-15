// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Images
{
    private static void CopyPixels(
        ImmutableArray<byte> data,
        uint stride
    ) { }

    internal static async Task Run(RuntimeHandle runtime)
    {
        // #region map
        var map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(800, 600, 2),
                MapMode = MapMode.Static,
            }
        );
        await map.Ready;
        // #endregion map

        // #region style
        map.Discarding.SetStyleUrl(
            "https://demotiles.maplibre.org/style.json"
        );
        // #endregion style

        // #region attach
        // With no device, the session creates its own.
        var session = map.Attach(
            new RenderTarget.Offscreen(
                new OffscreenTarget(new LogicalSize(800, 600))
            ),
            null,
            // The library renders on its own thread.
            new RenderSessionAttachOptions
            {
                Driver = RenderDriverKind.CoreWorker,
            }
        );
        await session.Ready;
        // #endregion attach

        // #region image
        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions
                {
                    Center = new LatLng(48.8566, 2.3522),
                    Zoom = 4,
                },
            }
        );

        await map.RequestStillImageAsync();
        // #endregion image

        // #region read
        var image = await session.ReadTextureAsync();
        CopyPixels(image.Data, image.Info.Stride);
        // #endregion read

        session.Dispose();
        await map.ReleaseAsync();
    }

    internal static async Task RunTile(
        RuntimeHandle runtime,
        uint z,
        uint x,
        uint y
    )
    {
        // #region tile
        var map = runtime.CreateMap(
            new MapOptions
            {
                InitialExtent = new LogicalExtent(512, 512, 1),
                MapMode = MapMode.Tile,
            }
        );
        await map.Ready;

        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = MaplibreNative.CameraForTile(
                    new CanonicalTileId(z, x, y),
                    new LogicalSize(512, 512)
                ),
            }
        );
        // #endregion tile

        await map.ReleaseAsync();
    }
}
