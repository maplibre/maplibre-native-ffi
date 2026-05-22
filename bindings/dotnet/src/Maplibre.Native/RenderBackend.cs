namespace Maplibre.Native;

/// <summary>Render backend support flags reported by the native library.</summary>
[Flags]
public enum RenderBackend : uint
{
    /// <summary>No render backend flags are set.</summary>
    None = 0,

    /// <summary>Metal rendering is available.</summary>
    Metal = 1u << 0,

    /// <summary>Vulkan rendering is available.</summary>
    Vulkan = 1u << 1,
}
