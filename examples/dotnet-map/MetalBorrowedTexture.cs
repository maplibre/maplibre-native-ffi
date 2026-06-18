using Maplibre.Native;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class MetalBorrowedTexture : IDisposable
{
    private readonly MetalContext context;
    private nint texture;

    public MetalBorrowedTexture(MetalContext context, Viewport viewport)
    {
        this.context = context;
        texture = context.CreateBorrowedTexture(viewport);
    }

    public NativePointer Pointer => NativePointer.FromBorrowedAddress(texture);

    public nint Texture => texture;

    public void Dispose()
    {
        if (texture == 0)
        {
            return;
        }

        context.ReleaseObject(texture);
        texture = 0;
    }
}
