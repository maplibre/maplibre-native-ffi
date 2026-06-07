using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal partial struct mln_metal_owned_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_metal_context_descriptor context;
  }

  internal unsafe partial struct mln_metal_borrowed_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public void* texture;
  }

  internal unsafe partial struct mln_metal_owned_texture_frame
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint64_t")]
    public ulong generation;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    public double scale_factor;

    [NativeTypeName("uint64_t")]
    public ulong frame_id;

    public void* texture;

    public void* device;

    [NativeTypeName("uint64_t")]
    public ulong pixel_format;
  }

  internal partial struct mln_vulkan_owned_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_vulkan_context_descriptor context;
  }

  internal unsafe partial struct mln_vulkan_borrowed_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_vulkan_context_descriptor context;

    public void* image;

    public void* image_view;

    [NativeTypeName("uint32_t")]
    public uint format;

    [NativeTypeName("uint32_t")]
    public uint initial_layout;

    [NativeTypeName("uint32_t")]
    public uint final_layout;
  }

  internal unsafe partial struct mln_vulkan_owned_texture_frame
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint64_t")]
    public ulong generation;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    public double scale_factor;

    [NativeTypeName("uint64_t")]
    public ulong frame_id;

    public void* image;

    public void* image_view;

    public void* device;

    [NativeTypeName("uint32_t")]
    public uint format;

    [NativeTypeName("uint32_t")]
    public uint layout;
  }

  internal partial struct mln_opengl_owned_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_opengl_context_descriptor context;
  }

  internal partial struct mln_opengl_borrowed_texture_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_render_target_extent extent;

    public mln_opengl_context_descriptor context;

    [NativeTypeName("uint32_t")]
    public uint texture;

    [NativeTypeName("uint32_t")]
    public uint target;
  }

  internal partial struct mln_opengl_owned_texture_frame
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint64_t")]
    public ulong generation;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    public double scale_factor;

    [NativeTypeName("uint64_t")]
    public ulong frame_id;

    [NativeTypeName("uint32_t")]
    public uint texture;

    [NativeTypeName("uint32_t")]
    public uint target;

    [NativeTypeName("uint32_t")]
    public uint internal_format;

    [NativeTypeName("uint32_t")]
    public uint format;

    [NativeTypeName("uint32_t")]
    public uint type;
  }

  internal partial struct mln_texture_image_info
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    [NativeTypeName("uint32_t")]
    public uint stride;

    [NativeTypeName("size_t")]
    public nuint byte_length;
  }

  internal static unsafe partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_metal_owned_texture_descriptor mln_metal_owned_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_metal_borrowed_texture_descriptor mln_metal_borrowed_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_vulkan_owned_texture_descriptor mln_vulkan_owned_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_vulkan_borrowed_texture_descriptor mln_vulkan_borrowed_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_opengl_owned_texture_descriptor mln_opengl_owned_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_opengl_borrowed_texture_descriptor mln_opengl_borrowed_texture_descriptor_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_texture_image_info mln_texture_image_info_default();

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_metal_owned_texture_attach(mln_map* map, [NativeTypeName("const mln_metal_owned_texture_descriptor *")] mln_metal_owned_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_metal_borrowed_texture_attach(mln_map* map, [NativeTypeName("const mln_metal_borrowed_texture_descriptor *")] mln_metal_borrowed_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_vulkan_owned_texture_attach(mln_map* map, [NativeTypeName("const mln_vulkan_owned_texture_descriptor *")] mln_vulkan_owned_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_vulkan_borrowed_texture_attach(mln_map* map, [NativeTypeName("const mln_vulkan_borrowed_texture_descriptor *")] mln_vulkan_borrowed_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_opengl_owned_texture_attach(mln_map* map, [NativeTypeName("const mln_opengl_owned_texture_descriptor *")] mln_opengl_owned_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_opengl_borrowed_texture_attach(mln_map* map, [NativeTypeName("const mln_opengl_borrowed_texture_descriptor *")] mln_opengl_borrowed_texture_descriptor* descriptor, mln_render_session** out_session);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_texture_read_premultiplied_rgba8(mln_render_session* session, [NativeTypeName("uint8_t *")] byte* out_data, [NativeTypeName("size_t")] nuint out_data_capacity, mln_texture_image_info* out_info);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_metal_owned_texture_acquire_frame(mln_render_session* session, mln_metal_owned_texture_frame* out_frame);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_metal_owned_texture_release_frame(mln_render_session* session, [NativeTypeName("const mln_metal_owned_texture_frame *")] mln_metal_owned_texture_frame* frame);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_vulkan_owned_texture_acquire_frame(mln_render_session* session, mln_vulkan_owned_texture_frame* out_frame);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_vulkan_owned_texture_release_frame(mln_render_session* session, [NativeTypeName("const mln_vulkan_owned_texture_frame *")] mln_vulkan_owned_texture_frame* frame);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_opengl_owned_texture_acquire_frame(mln_render_session* session, mln_opengl_owned_texture_frame* out_frame);

    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern mln_status mln_opengl_owned_texture_release_frame(mln_render_session* session, [NativeTypeName("const mln_opengl_owned_texture_frame *")] mln_opengl_owned_texture_frame* frame);
  }
}
