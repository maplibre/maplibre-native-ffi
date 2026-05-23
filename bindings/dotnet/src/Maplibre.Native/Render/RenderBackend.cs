namespace Maplibre.Native.Render;

/// <summary>Render backend support flags reported by the native library.</summary>
[Flags]
public enum RenderBackend : uint
{
    None = 0,
    Metal = 1u << 0,
    Vulkan = 1u << 1,
}

/// <summary>Frame-scoped borrowed native pointer.</summary>
public readonly record struct FrameNativePointer(NativePointer Pointer);
