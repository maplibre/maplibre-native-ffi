using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.C
{
  internal partial struct mln_render_target_extent
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    [NativeTypeName("uint32_t")]
    public uint width;

    [NativeTypeName("uint32_t")]
    public uint height;

    public double scale_factor;
  }

  internal unsafe partial struct mln_metal_context_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public void* device;
  }

  internal unsafe partial struct mln_vulkan_context_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public void* instance;

    public void* physical_device;

    public void* device;

    public void* graphics_queue;

    [NativeTypeName("uint32_t")]
    public uint graphics_queue_family_index;

    public void* get_instance_proc_addr;

    public void* get_device_proc_addr;
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_opengl_context_provider_flag : uint
  {
    MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL = 1U << 0,
    MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL = 1U << 1,
  }

  [NativeTypeName("uint32_t")]
  internal enum mln_opengl_context_platform : uint
  {
    MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED = 0U,
    MLN_OPENGL_CONTEXT_PLATFORM_WGL = 1U,
    MLN_OPENGL_CONTEXT_PLATFORM_EGL = 2U,
  }

  internal unsafe partial struct mln_wgl_context_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public void* device_context;

    public void* share_context;

    public void* get_proc_address;
  }

  internal unsafe partial struct mln_egl_context_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public void* display;

    public void* config;

    public void* share_context;

    public void* get_proc_address;
  }

  internal partial struct mln_opengl_context_descriptor
  {
    [NativeTypeName("uint32_t")]
    public uint size;

    public mln_opengl_context_platform platform;

    [NativeTypeName("__AnonymousRecord_render_target_L104_C3")]
    public _data_e__Union data;

    [StructLayout(LayoutKind.Explicit)]
    internal partial struct _data_e__Union
    {
      [FieldOffset(0)]
      public mln_wgl_context_descriptor wgl;

      [FieldOffset(0)]
      public mln_egl_context_descriptor egl;
    }
  }

  internal static partial class NativeMethods
  {
    [DllImport("maplibre-native-c", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint32_t")]
    public static extern uint mln_opengl_supported_context_provider_mask();
  }
}
