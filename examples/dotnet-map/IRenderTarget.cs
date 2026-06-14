namespace Maplibre.Native.Examples.DotnetMap;

internal interface IRenderTarget : IDisposable
{
    bool NeedsReattachOnResize { get; }

    void Render();

    void Resize(Viewport viewport);
}

internal static class RenderTargetFactory
{
    public static IRenderTarget Attach(
        IGraphicsContext graphics,
        MapState mapState,
        RenderTargetMode mode
    )
    {
        ArgumentNullException.ThrowIfNull(graphics);
        ArgumentNullException.ThrowIfNull(mapState);

        return mode.Kind switch
        {
            RenderTargetModeKind.OwnedTexture => AttachOwnedTexture(graphics, mapState),
            RenderTargetModeKind.BorrowedTexture => AttachBorrowedTexture(graphics, mapState),
            RenderTargetModeKind.NativeSurface => AttachNativeSurface(graphics, mapState),
            _ => throw new ArgumentOutOfRangeException(nameof(mode)),
        };
    }

    private static IRenderTarget AttachOwnedTexture(IGraphicsContext graphics, MapState mapState)
    {
        _ = graphics;
        _ = mapState;
        throw new NotImplementedException(
            "owned-texture render target attachment is not implemented yet."
        );
    }

    private static IRenderTarget AttachBorrowedTexture(IGraphicsContext graphics, MapState mapState)
    {
        _ = graphics;
        _ = mapState;
        throw new NotImplementedException(
            "borrowed-texture render target attachment is not implemented yet."
        );
    }

    private static IRenderTarget AttachNativeSurface(IGraphicsContext graphics, MapState mapState)
    {
        _ = graphics;
        _ = mapState;
        throw new NotImplementedException(
            "native-surface render target attachment is not implemented yet."
        );
    }
}
