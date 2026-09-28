namespace Maplibre.NativeFfi.Runtime;

public sealed partial class RuntimeHandle : IAsyncDisposable
{
    public ValueTask DisposeAsync() => new(CloseAsync());
}
