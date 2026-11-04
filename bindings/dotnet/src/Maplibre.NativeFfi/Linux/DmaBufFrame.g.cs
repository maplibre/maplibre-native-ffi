// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The dmabuf of an acquired frame.
/// </summary>
/// <remarks>
/// See <c>mln_dma_buf_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Slot">
/// The buffer's index in the ring.
/// </param>
/// <param name="Offset">
/// The bytes from the start of the buffer to its first pixel.
/// </param>
/// <param name="Stride">
/// The bytes from the start of one row to the start of the next.
/// </param>
public readonly partial record struct DmaBufFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    uint Fourcc,
    ulong Modifier,
    int Fd,
    uint Offset,
    uint Stride
);
