using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal interface IGraphicsContext : IDisposable
{
    RenderBackendFlag Backend { get; }

    nint WindowHandle { get; }

    GlfwWindow Window { get; }

    bool ShouldClose { get; }

    bool CanRenderFrame { get; }

    Viewport ReadViewport();

    void Resize(Viewport viewport);

    void FinishFrame();
}

internal static class GraphicsContext
{
    public static IGraphicsContext Create(
        string title,
        int width,
        int height,
        RenderBackendFlag backends,
        bool visible
    )
    {
        if (backends.HasFlag(RenderBackendFlag.Metal))
        {
            return MetalContext.Create(title, width, height, visible);
        }

        if (backends.HasFlag(RenderBackendFlag.Opengl))
        {
            return OpenGLContext.Create(title, width, height, visible);
        }

        if (backends.HasFlag(RenderBackendFlag.Vulkan))
        {
            return VulkanContext.Create(title, width, height, visible);
        }

        throw new InvalidOperationException(
            "The loaded MapLibre native library does not support a backend usable by dotnet-map."
        );
    }
}
