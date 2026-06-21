namespace Maplibre.Native.Examples.DotnetMap;

internal enum RenderTargetModeKind
{
    OwnedTexture,
    BorrowedTexture,
    NativeSurface,
}

internal readonly record struct RenderTargetMode(
    RenderTargetModeKind Kind,
    string CliName,
    string Status
)
{
    public static readonly RenderTargetMode OwnedTexture = new(
        RenderTargetModeKind.OwnedTexture,
        "owned-texture",
        "samples MapLibre-owned texture frames into the host swapchain"
    );

    public static readonly RenderTargetMode BorrowedTexture = new(
        RenderTargetModeKind.BorrowedTexture,
        "borrowed-texture",
        "renders into a host-owned texture, then samples it into the host swapchain"
    );

    public static readonly RenderTargetMode NativeSurface = new(
        RenderTargetModeKind.NativeSurface,
        "native-surface",
        "renders directly to the host window surface"
    );

    public static IReadOnlyList<RenderTargetMode> All { get; } =
    [OwnedTexture, BorrowedTexture, NativeSurface];

    public static bool TryParse(string value, out RenderTargetMode mode)
    {
        foreach (var candidate in All)
        {
            if (StringComparer.Ordinal.Equals(candidate.CliName, value))
            {
                mode = candidate;
                return true;
            }
        }

        mode = default;
        return false;
    }
}
