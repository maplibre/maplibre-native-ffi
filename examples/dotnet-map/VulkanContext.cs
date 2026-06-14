using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class VulkanContext : IGraphicsContext
{
    private VulkanContext() { }

    public RenderBackend Backend => RenderBackend.Vulkan;

    public nint WindowHandle => throw new NotImplementedException();

    public static VulkanContext Create(string title, int width, int height)
    {
        _ = title;
        _ = width;
        _ = height;
        throw new NotImplementedException(
            "Vulkan graphics context creation is not implemented yet."
        );
    }

    public Viewport ReadViewport()
    {
        throw new NotImplementedException("Vulkan viewport reads are not implemented yet.");
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException("Vulkan resize is not implemented yet.");
    }

    public void FinishFrame()
    {
        throw new NotImplementedException("Vulkan frame maintenance is not implemented yet.");
    }

    public void Dispose() { }
}
