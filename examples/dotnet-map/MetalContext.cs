using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MetalContext : IGraphicsContext
{
    private MetalContext() { }

    public RenderBackend Backend => RenderBackend.Metal;

    public nint WindowHandle => throw new NotImplementedException();

    public static MetalContext Create(string title, int width, int height)
    {
        _ = title;
        _ = width;
        _ = height;
        throw new NotImplementedException(
            "Metal graphics context creation is not implemented yet."
        );
    }

    public Viewport ReadViewport()
    {
        throw new NotImplementedException("Metal viewport reads are not implemented yet.");
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException("Metal resize is not implemented yet.");
    }

    public void FinishFrame()
    {
        throw new NotImplementedException("Metal frame maintenance is not implemented yet.");
    }

    public void Dispose() { }
}
