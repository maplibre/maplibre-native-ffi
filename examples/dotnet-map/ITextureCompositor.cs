namespace Maplibre.Native.Examples.DotnetMap;

internal interface ITextureCompositor : IDisposable
{
    void Resize(Viewport viewport);

    void Draw();
}
