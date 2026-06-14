namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class VulkanTextureCompositor : ITextureCompositor
{
    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException(
            "Vulkan texture compositor resize is not implemented yet."
        );
    }

    public void Draw()
    {
        throw new NotImplementedException("Vulkan texture compositor draw is not implemented yet.");
    }

    public void Dispose() { }
}
