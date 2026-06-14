using Maplibre.Native.Map;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MapState : IDisposable
{
    private MapState() { }

    private MapHandle? MapOrNull { get; set; }

    public MapHandle Map =>
        MapOrNull ?? throw new InvalidOperationException("Map state has not created a map yet.");

    public bool RenderPending { get; private set; }

    public static MapState Create(
        IGraphicsContext graphics,
        Viewport viewport,
        RenderTargetMode renderTargetMode
    )
    {
        ArgumentNullException.ThrowIfNull(graphics);
        _ = viewport;
        _ = renderTargetMode;
        throw new NotImplementedException("Map state creation is not implemented yet.");
    }

    public void RequestRender()
    {
        RenderPending = true;
    }

    public bool Step()
    {
        throw new NotImplementedException("Map frame stepping is not implemented yet.");
    }

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        throw new NotImplementedException("Map resize is not implemented yet.");
    }

    public void Dispose() { }
}
