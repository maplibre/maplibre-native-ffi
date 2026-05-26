use std::ffi::c_void;

use maplibre_native_sys as sys;

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct RenderTargetExtentFields {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalContextDescriptorFields {
    pub device: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanContextDescriptorFields {
    pub instance: *mut c_void,
    pub physical_device: *mut c_void,
    pub device: *mut c_void,
    pub graphics_queue: *mut c_void,
    pub graphics_queue_family_index: u32,
    pub get_instance_proc_addr: *mut c_void,
    pub get_device_proc_addr: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WglContextDescriptorFields {
    pub device_context: *mut c_void,
    pub share_context: *mut c_void,
    pub get_proc_address: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct EglContextDescriptorFields {
    pub display: *mut c_void,
    pub config: *mut c_void,
    pub share_context: *mut c_void,
    pub get_proc_address: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum OpenGLContextDescriptorFields {
    Wgl(WglContextDescriptorFields),
    Egl(EglContextDescriptorFields),
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: MetalContextDescriptorFields,
    pub layer: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: VulkanContextDescriptorFields,
    pub surface: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGLSurfaceDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: OpenGLContextDescriptorFields,
    pub surface: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: MetalContextDescriptorFields,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalBorrowedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub texture: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: VulkanContextDescriptorFields,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanBorrowedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: VulkanContextDescriptorFields,
    pub image: *mut c_void,
    pub image_view: *mut c_void,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGLOwnedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: OpenGLContextDescriptorFields,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGLBorrowedTextureDescriptorFields {
    pub extent: RenderTargetExtentFields,
    pub context: OpenGLContextDescriptorFields,
    pub texture: u32,
    pub target: u32,
}

fn render_target_extent_to_native(
    fields: RenderTargetExtentFields,
) -> sys::mln_render_target_extent {
    sys::mln_render_target_extent {
        size: std::mem::size_of::<sys::mln_render_target_extent>() as u32,
        width: fields.width,
        height: fields.height,
        scale_factor: fields.scale_factor,
    }
}

fn metal_context_descriptor_to_native(
    fields: MetalContextDescriptorFields,
) -> sys::mln_metal_context_descriptor {
    sys::mln_metal_context_descriptor {
        size: std::mem::size_of::<sys::mln_metal_context_descriptor>() as u32,
        device: fields.device,
    }
}

fn vulkan_context_descriptor_to_native(
    fields: VulkanContextDescriptorFields,
) -> sys::mln_vulkan_context_descriptor {
    sys::mln_vulkan_context_descriptor {
        size: std::mem::size_of::<sys::mln_vulkan_context_descriptor>() as u32,
        instance: fields.instance,
        physical_device: fields.physical_device,
        device: fields.device,
        graphics_queue: fields.graphics_queue,
        graphics_queue_family_index: fields.graphics_queue_family_index,
        get_instance_proc_addr: fields.get_instance_proc_addr,
        get_device_proc_addr: fields.get_device_proc_addr,
    }
}

fn wgl_context_descriptor_to_native(
    fields: WglContextDescriptorFields,
) -> sys::mln_wgl_context_descriptor {
    sys::mln_wgl_context_descriptor {
        size: std::mem::size_of::<sys::mln_wgl_context_descriptor>() as u32,
        device_context: fields.device_context,
        share_context: fields.share_context,
        get_proc_address: fields.get_proc_address,
    }
}

fn egl_context_descriptor_to_native(
    fields: EglContextDescriptorFields,
) -> sys::mln_egl_context_descriptor {
    sys::mln_egl_context_descriptor {
        size: std::mem::size_of::<sys::mln_egl_context_descriptor>() as u32,
        display: fields.display,
        config: fields.config,
        share_context: fields.share_context,
        get_proc_address: fields.get_proc_address,
    }
}

fn opengl_context_descriptor_to_native(
    fields: OpenGLContextDescriptorFields,
) -> sys::mln_opengl_context_descriptor {
    match fields {
        OpenGLContextDescriptorFields::Wgl(wgl) => sys::mln_opengl_context_descriptor {
            size: std::mem::size_of::<sys::mln_opengl_context_descriptor>() as u32,
            platform: sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL,
            data: sys::mln_opengl_context_descriptor__bindgen_ty_1 {
                wgl: wgl_context_descriptor_to_native(wgl),
            },
        },
        OpenGLContextDescriptorFields::Egl(egl) => sys::mln_opengl_context_descriptor {
            size: std::mem::size_of::<sys::mln_opengl_context_descriptor>() as u32,
            platform: sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL,
            data: sys::mln_opengl_context_descriptor__bindgen_ty_1 {
                egl: egl_context_descriptor_to_native(egl),
            },
        },
    }
}

pub fn metal_surface_descriptor_to_native(
    fields: MetalSurfaceDescriptorFields,
) -> sys::mln_metal_surface_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_metal_surface_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = metal_context_descriptor_to_native(fields.context);
    raw.layer = fields.layer;
    raw
}

pub fn vulkan_surface_descriptor_to_native(
    fields: VulkanSurfaceDescriptorFields,
) -> sys::mln_vulkan_surface_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = vulkan_context_descriptor_to_native(fields.context);
    raw.surface = fields.surface;
    raw
}

pub fn opengl_surface_descriptor_to_native(
    fields: OpenGLSurfaceDescriptorFields,
) -> sys::mln_opengl_surface_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_opengl_surface_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = opengl_context_descriptor_to_native(fields.context);
    raw.surface = fields.surface;
    raw
}

pub fn metal_owned_texture_descriptor_to_native(
    fields: MetalOwnedTextureDescriptorFields,
) -> sys::mln_metal_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = metal_context_descriptor_to_native(fields.context);
    raw
}

pub fn metal_borrowed_texture_descriptor_to_native(
    fields: MetalBorrowedTextureDescriptorFields,
) -> sys::mln_metal_borrowed_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.texture = fields.texture;
    raw
}

pub fn vulkan_owned_texture_descriptor_to_native(
    fields: VulkanOwnedTextureDescriptorFields,
) -> sys::mln_vulkan_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = vulkan_context_descriptor_to_native(fields.context);
    raw
}

pub fn vulkan_borrowed_texture_descriptor_to_native(
    fields: VulkanBorrowedTextureDescriptorFields,
) -> sys::mln_vulkan_borrowed_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = vulkan_context_descriptor_to_native(fields.context);
    raw.image = fields.image;
    raw.image_view = fields.image_view;
    raw.format = fields.format;
    raw.initial_layout = fields.initial_layout;
    raw.final_layout = fields.final_layout;
    raw
}

pub fn opengl_owned_texture_descriptor_to_native(
    fields: OpenGLOwnedTextureDescriptorFields,
) -> sys::mln_opengl_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = opengl_context_descriptor_to_native(fields.context);
    raw
}

pub fn opengl_borrowed_texture_descriptor_to_native(
    fields: OpenGLBorrowedTextureDescriptorFields,
) -> sys::mln_opengl_borrowed_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size fields.
    let mut raw = unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    raw.extent = render_target_extent_to_native(fields.extent);
    raw.context = opengl_context_descriptor_to_native(fields.context);
    raw.texture = fields.texture;
    raw.target = fields.target;
    raw
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ptr(address: usize) -> *mut c_void {
        address as *mut c_void
    }

    fn extent() -> RenderTargetExtentFields {
        RenderTargetExtentFields {
            width: 32,
            height: 16,
            scale_factor: 2.0,
        }
    }

    fn metal_context(address: usize) -> MetalContextDescriptorFields {
        MetalContextDescriptorFields {
            device: ptr(address),
        }
    }

    fn vulkan_context(base: usize) -> VulkanContextDescriptorFields {
        VulkanContextDescriptorFields {
            instance: ptr(base),
            physical_device: ptr(base + 1),
            device: ptr(base + 2),
            graphics_queue: ptr(base + 3),
            graphics_queue_family_index: base as u32 + 4,
            get_instance_proc_addr: ptr(base + 20),
            get_device_proc_addr: ptr(base + 21),
        }
    }

    fn wgl_context(base: usize) -> OpenGLContextDescriptorFields {
        OpenGLContextDescriptorFields::Wgl(WglContextDescriptorFields {
            device_context: ptr(base),
            share_context: ptr(base + 1),
            get_proc_address: ptr(base + 2),
        })
    }

    fn egl_context(base: usize) -> OpenGLContextDescriptorFields {
        OpenGLContextDescriptorFields::Egl(EglContextDescriptorFields {
            display: ptr(base),
            config: ptr(base + 1),
            share_context: ptr(base + 2),
            get_proc_address: ptr(base + 3),
        })
    }

    #[test]
    fn metal_descriptors_fill_sizes_fields_and_pointers() {
        let surface = metal_surface_descriptor_to_native(MetalSurfaceDescriptorFields {
            extent: extent(),
            context: metal_context(2),
            layer: ptr(1),
        });
        assert_eq!(
            surface.size,
            std::mem::size_of::<sys::mln_metal_surface_descriptor>() as u32
        );
        assert_eq!(
            surface.context.size,
            std::mem::size_of::<sys::mln_metal_context_descriptor>() as u32
        );
        assert_eq!(surface.layer, ptr(1));
        assert_eq!(surface.context.device, ptr(2));

        let owned_texture =
            metal_owned_texture_descriptor_to_native(MetalOwnedTextureDescriptorFields {
                extent: extent(),
                context: metal_context(3),
            });
        assert_eq!(
            owned_texture.size,
            std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned_texture.context.device, ptr(3));

        let borrowed =
            metal_borrowed_texture_descriptor_to_native(MetalBorrowedTextureDescriptorFields {
                extent: extent(),
                texture: ptr(4),
            });
        assert_eq!(
            borrowed.size,
            std::mem::size_of::<sys::mln_metal_borrowed_texture_descriptor>() as u32
        );
        assert_eq!(borrowed.texture, ptr(4));
    }

    #[test]
    fn vulkan_descriptors_fill_sizes_fields_and_pointers() {
        let surface = vulkan_surface_descriptor_to_native(VulkanSurfaceDescriptorFields {
            extent: extent(),
            context: vulkan_context(1),
            surface: ptr(6),
        });
        assert_eq!(
            surface.size,
            std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as u32
        );
        assert_eq!(surface.context.instance, ptr(1));
        assert_eq!(surface.context.graphics_queue_family_index, 5);
        assert_eq!(surface.context.get_instance_proc_addr, ptr(21));
        assert_eq!(surface.context.get_device_proc_addr, ptr(22));
        assert_eq!(surface.surface, ptr(6));

        let owned = vulkan_owned_texture_descriptor_to_native(VulkanOwnedTextureDescriptorFields {
            extent: extent(),
            context: vulkan_context(7),
        });
        assert_eq!(
            owned.size,
            std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned.context.instance, ptr(7));
        assert_eq!(owned.context.graphics_queue_family_index, 11);
        assert_eq!(owned.context.get_instance_proc_addr, ptr(27));
        assert_eq!(owned.context.get_device_proc_addr, ptr(28));

        let borrowed =
            vulkan_borrowed_texture_descriptor_to_native(VulkanBorrowedTextureDescriptorFields {
                extent: extent(),
                context: vulkan_context(12),
                image: ptr(17),
                image_view: ptr(18),
                format: 19,
                initial_layout: 20,
                final_layout: 21,
            });
        assert_eq!(
            borrowed.size,
            std::mem::size_of::<sys::mln_vulkan_borrowed_texture_descriptor>() as u32
        );
        assert_eq!(borrowed.image, ptr(17));
        assert_eq!(borrowed.image_view, ptr(18));
        assert_eq!(borrowed.format, 19);
        assert_eq!(borrowed.initial_layout, 20);
        assert_eq!(borrowed.final_layout, 21);
    }

    #[test]
    fn opengl_descriptors_fill_sizes_fields_and_provider_union() {
        let surface = opengl_surface_descriptor_to_native(OpenGLSurfaceDescriptorFields {
            extent: extent(),
            context: wgl_context(1),
            surface: ptr(6),
        });
        assert_eq!(
            surface.size,
            std::mem::size_of::<sys::mln_opengl_surface_descriptor>() as u32
        );
        assert_eq!(
            surface.context.platform,
            sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL
        );
        // SAFETY: platform selects the wgl union field initialized above.
        assert_eq!(unsafe { surface.context.data.wgl.device_context }, ptr(1));
        assert_eq!(unsafe { surface.context.data.wgl.share_context }, ptr(2));
        assert_eq!(unsafe { surface.context.data.wgl.get_proc_address }, ptr(3));
        assert_eq!(surface.surface, ptr(6));

        let owned = opengl_owned_texture_descriptor_to_native(OpenGLOwnedTextureDescriptorFields {
            extent: extent(),
            context: egl_context(7),
        });
        assert_eq!(
            owned.size,
            std::mem::size_of::<sys::mln_opengl_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned.context.platform, sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL);
        // SAFETY: platform selects the egl union field initialized above.
        assert_eq!(unsafe { owned.context.data.egl.display }, ptr(7));
        assert_eq!(unsafe { owned.context.data.egl.config }, ptr(8));
        assert_eq!(unsafe { owned.context.data.egl.share_context }, ptr(9));
        assert_eq!(unsafe { owned.context.data.egl.get_proc_address }, ptr(10));

        let borrowed =
            opengl_borrowed_texture_descriptor_to_native(OpenGLBorrowedTextureDescriptorFields {
                extent: extent(),
                context: wgl_context(11),
                texture: 12,
                target: 0x0de1,
            });
        assert_eq!(
            borrowed.size,
            std::mem::size_of::<sys::mln_opengl_borrowed_texture_descriptor>() as u32
        );
        assert_eq!(borrowed.texture, 12);
        assert_eq!(borrowed.target, 0x0de1);
    }
}
