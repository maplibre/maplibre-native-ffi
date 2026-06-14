using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class OpenGLContext : IGraphicsContext
{
    private OpenGLContext() { }

    public RenderBackend Backend => RenderBackend.OpenGL;

    public nint WindowHandle => throw new NotImplementedException();

    public static OpenGLContext Create(string title, int width, int height)
    {
        _ = title;
        _ = width;
        _ = height;
        throw new NotImplementedException(
            "OpenGL graphics context creation is not implemented yet."
        );
    }

    public Viewport ReadViewport()
    {
        throw new NotImplementedException("OpenGL viewport reads are not implemented yet.");
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException("OpenGL resize is not implemented yet.");
    }

    public void FinishFrame()
    {
        throw new NotImplementedException("OpenGL frame maintenance is not implemented yet.");
    }

    public void Dispose() { }
}
