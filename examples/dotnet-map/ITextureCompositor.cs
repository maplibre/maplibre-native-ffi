namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal interface ITextureCompositor : IDisposable
{
    void Resize(Viewport viewport);

    bool Draw(MetalTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Metal texture frames are not supported by this compositor."
        );
    }

    bool Draw(VulkanTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "Vulkan texture frames are not supported by this compositor."
        );
    }

    bool Draw(OpenglTextureFrameView frame)
    {
        _ = frame;
        throw new NotSupportedException(
            "OpenGL texture frames are not supported by this compositor."
        );
    }
}
