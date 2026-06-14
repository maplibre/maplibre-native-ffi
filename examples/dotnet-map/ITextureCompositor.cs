using Maplibre.Native.Render;

namespace Maplibre.Native.Examples.DotnetMap;

internal interface ITextureCompositor : IDisposable
{
    void Resize(Viewport viewport);

    void Draw(MetalOwnedTextureFrame frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Metal texture frames are not supported by this compositor."
        );
    }

    void Draw(VulkanOwnedTextureFrame frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Vulkan texture frames are not supported by this compositor."
        );
    }

    void Draw(OpenGLOwnedTextureFrame frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "OpenGL texture frames are not supported by this compositor."
        );
    }
}
