using Maplibre.NativeFfi.Render;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal interface ITextureCompositor : IDisposable
{
    void Resize(Viewport viewport);

    bool Draw(MetalOwnedTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Metal texture frames are not supported by this compositor."
        );
    }

    bool Draw(VulkanOwnedTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Vulkan texture frames are not supported by this compositor."
        );
    }

    bool Draw(OpenglOwnedTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "OpenGL texture frames are not supported by this compositor."
        );
    }
}
