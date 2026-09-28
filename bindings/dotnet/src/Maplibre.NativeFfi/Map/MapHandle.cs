namespace Maplibre.NativeFfi.Map;

public sealed partial class MapHandle : IAsyncDisposable
{
    public ValueTask DisposeAsync() => new(CloseAsync());
}
