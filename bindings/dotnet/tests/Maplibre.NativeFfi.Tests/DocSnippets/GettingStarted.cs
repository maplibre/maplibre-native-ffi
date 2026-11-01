// Snippets for the docs site's Getting started page. They compile with the
// tests, which never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class GettingStarted
{
    private static void CopyPixels(ImmutableArray<byte> data, uint stride) { }

    private static async Task Run()
    {
        // #region runtime
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        // #endregion runtime

        // #region map
        var extent = new LogicalExtent(512, 512, 1);
        var map = runtime.CreateMap(
            new MapOptions { InitialExtent = extent, MapMode = MapMode.Static }
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

        // #region render
        await map.RequestStillImageAsync();
        // #endregion render

        // #region render
        var readback = await session.ReadTextureAsync();
        // #endregion render

        // #region render
        CopyPixels(readback.Data, readback.Info.Stride);
        // #endregion render

        // #region close
        session.Dispose();
        device.Dispose();
        await map.CloseAsync();
        await runtime.CloseAsync();
        // #endregion close
    }
}
