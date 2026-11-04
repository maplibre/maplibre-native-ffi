// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A DRM format and modifier that you can import.
/// </summary>
/// <remarks>
/// See <c>mln_dma_buf_format</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Fourcc">
/// A DRM fourcc code, such as DRM_FORMAT_ABGR8888.
/// </param>
/// <param name="Modifier">
/// A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR.
/// </param>
public readonly partial record struct DmaBufFormat(uint Fourcc, ulong Modifier);
