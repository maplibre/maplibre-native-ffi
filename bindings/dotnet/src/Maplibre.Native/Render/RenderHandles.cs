namespace Maplibre.Native.Render;

public sealed class RenderSessionHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public void Close() => IsClosed = true;
    public void Dispose() => Close();
}

public sealed class MetalOwnedTextureFrameHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public MetalOwnedTextureFrame Frame => IsClosed ? throw new ObjectDisposedException(nameof(MetalOwnedTextureFrameHandle)) : default;
    public void Dispose() => IsClosed = true;
}

public sealed class VulkanOwnedTextureFrameHandle : IDisposable
{
    public bool IsClosed { get; private set; }
    public VulkanOwnedTextureFrame Frame => IsClosed ? throw new ObjectDisposedException(nameof(VulkanOwnedTextureFrameHandle)) : default;
    public void Dispose() => IsClosed = true;
}

public sealed class FrameScope : IDisposable
{
    public bool IsClosed { get; private set; }
    public void Dispose() => IsClosed = true;
}
