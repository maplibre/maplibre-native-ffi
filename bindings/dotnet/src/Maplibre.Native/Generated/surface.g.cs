using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal unsafe partial struct mln_metal_surface_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_metal_context_descriptor context;

    public void* layer;
  }

  internal unsafe partial struct mln_vulkan_surface_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_vulkan_context_descriptor context;

    public void* surface;
  }

  internal unsafe partial struct mln_opengl_surface_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_opengl_context_descriptor context;

    public void* surface;
  }

  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_metal_surface_descriptor mln_metal_surface_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_vulkan_surface_descriptor mln_vulkan_surface_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_opengl_surface_descriptor mln_opengl_surface_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_metal_surface_attach(mln_map* map, [NativeTypeName("const mln_metal_surface_descriptor *")] mln_metal_surface_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_vulkan_surface_attach(mln_map* map, [NativeTypeName("const mln_vulkan_surface_descriptor *")] mln_vulkan_surface_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_opengl_surface_attach(mln_map* map, [NativeTypeName("const mln_opengl_surface_descriptor *")] mln_opengl_surface_descriptor* descriptor, mln_render_session** out_session);
  }
}
