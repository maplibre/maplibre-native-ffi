// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Where one plane of a dmabuf keeps its data.
/// </summary>
/// <remarks>
/// See <c>mln_dma_buf_plane</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Fd">
/// The file descriptor of the dmabuf that holds the plane. The planes of a
/// buffer often share one.
/// </param>
/// <param name="Offset">
/// The bytes from the start of the dmabuf to the plane's first byte.
/// </param>
/// <param name="Stride">
/// The bytes from the start of one row to the start of the next.
/// </param>
public readonly partial record struct DmaBufPlane(int Fd, uint Offset, uint Stride);
