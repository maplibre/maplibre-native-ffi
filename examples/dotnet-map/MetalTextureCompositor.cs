namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MetalTextureCompositor : ITextureCompositor
{
    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException(
            "Metal texture compositor resize is not implemented yet."
        );
    }

    public void Draw()
    {
        throw new NotImplementedException("Metal texture compositor draw is not implemented yet.");
    }

    public void Dispose() { }
}
