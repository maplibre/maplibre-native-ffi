using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal interface IGraphicsContext : IDisposable
{
    RenderBackend Backend { get; }

    nint WindowHandle { get; }

    GlfwWindow Window { get; }

    bool ShouldClose { get; }

    bool CanRenderFrame { get; }

    Viewport ReadViewport();

    void Resize(Viewport viewport);

    void PollEvents();

    void FinishFrame();
}

internal static class GraphicsContext
{
    public static IGraphicsContext Create(
        string title,
        int width,
        int height,
        RenderBackend backends
    )
    {
        if (OperatingSystem.IsMacOS() && backends.HasFlag(RenderBackend.Metal))
        {
            return MetalContext.Create(title, width, height);
        }

        if (!OperatingSystem.IsMacOS() && backends.HasFlag(RenderBackend.OpenGL))
        {
            return OpenGLContext.Create(title, width, height);
        }

        if (backends.HasFlag(RenderBackend.Vulkan))
        {
            return VulkanContext.Create(title, width, height);
        }

        throw new InvalidOperationException(
            "The loaded MapLibre native library does not support a backend usable by dotnet-map on this platform."
        );
    }
}
