// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Images
{
    // The center of tile z/x/y. The snippet leaves the tile math to the reader.
    private static LatLng TileCenter(uint z, uint x, uint y) => default;

    private static void CopyPixels(ImmutableArray<byte> data, uint stride) { }

    internal static async Task Run(RuntimeHandle runtime, uint z, uint x, uint y)
    {
        // #region map
        var extent = new LogicalExtent(512, 512, 1);
        var map = runtime.CreateMap(
            new MapOptions { InitialExtent = extent, MapMode = MapMode.Tile }
        );
        await map.Ready;
        // #endregion map

        // #region style
        map.Discarding.SetStyleUrl("https://demotiles.maplibre.org/style.json");
        // #endregion style

        // #region attach
        var device = GraphicsDeviceHandle.Create();

        var session = map.AttachGraphicsDeviceTexture(
            device,
            new GraphicsDeviceTextureDescriptor(extent),
            // The library renders on its own thread.
            new RenderSessionAttachOptions { Driver = RenderDriverKind.CoreWorker }
        );
        await session.Ready;
        // #endregion attach

        // #region image
        map.Discarding.UpdateCamera(
            new CameraUpdate
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Center = TileCenter(z, x, y), Zoom = z },
            }
        );

        await map.RequestStillImageAsync();
        // #endregion image

        // #region read
        var image = await session.ReadTextureAsync();
        CopyPixels(image.Data, image.Info.Stride);
        // #endregion read

        // #region close
        session.Dispose();
        device.Dispose();
        await map.CloseAsync();
        // #endregion close
    }
}
