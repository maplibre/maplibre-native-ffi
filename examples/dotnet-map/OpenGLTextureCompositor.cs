namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class OpenGLTextureCompositor : ITextureCompositor
{
    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException(
            "OpenGL texture compositor resize is not implemented yet."
        );
    }

    public void Draw()
    {
        throw new NotImplementedException("OpenGL texture compositor draw is not implemented yet.");
    }

    public void Dispose() { }
}
