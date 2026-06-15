namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class OpenGLBorrowedTexture : IDisposable
{
    public const uint Texture2D = 0x0DE1;

    private const uint TextureMinFilter = 0x2801;
    private const uint TextureMagFilter = 0x2800;
    private const int Rgba8 = 0x8058;
    private const uint Rgba = 0x1908;
    private const uint UnsignedByte = 0x1401;
    private const int Linear = 0x2601;

    private readonly OpenGLContext context;
    private uint texture;

    public OpenGLBorrowedTexture(OpenGLContext context, Viewport viewport)
    {
        this.context = context;
        context.MakeCurrentForRendering();
        texture = context.GenTexture();
        try
        {
            context.BindTexture(Texture2D, texture);
            context.TexParameter(Texture2D, TextureMinFilter, Linear);
            context.TexParameter(Texture2D, TextureMagFilter, Linear);
            context.TexImage2D(
                Texture2D,
                0,
                Rgba8,
                viewport.PhysicalWidth,
                viewport.PhysicalHeight,
                0,
                Rgba,
                UnsignedByte
            );
            context.BindTexture(Texture2D, 0);
            CheckError("create OpenGL borrowed texture");
        }
        catch
        {
            context.BindTexture(Texture2D, 0);
            context.DeleteTexture(texture);
            texture = 0;
            throw;
        }
    }

    public uint Texture => texture;

    public uint Target => Texture2D;

    public void Dispose()
    {
        if (texture == 0)
        {
            return;
        }

        context.MakeCurrentForRendering();
        context.DeleteTexture(texture);
        texture = 0;
    }

    private void CheckError(string operation)
    {
        var error = context.GetError();
        if (error != 0)
        {
            throw new InvalidOperationException(
                $"{operation} failed with OpenGL error 0x{error:x}"
            );
        }
    }
}
