use std::ffi::c_void;

use maplibre_native_sys as sys;

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TextureDescriptorFields {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalSurfaceDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub layer: *mut c_void,
    pub device: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanSurfaceDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub instance: *mut c_void,
    pub physical_device: *mut c_void,
    pub device: *mut c_void,
    pub graphics_queue: *mut c_void,
    pub graphics_queue_family_index: u32,
    pub surface: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub device: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalBorrowedTextureDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub texture_handle: *mut c_void,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub instance: *mut c_void,
    pub physical_device: *mut c_void,
    pub device: *mut c_void,
    pub graphics_queue: *mut c_void,
    pub graphics_queue_family_index: u32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanBorrowedTextureDescriptorFields {
    pub texture: TextureDescriptorFields,
    pub instance: *mut c_void,
    pub physical_device: *mut c_void,
    pub device: *mut c_void,
    pub graphics_queue: *mut c_void,
    pub graphics_queue_family_index: u32,
    pub image: *mut c_void,
    pub image_view: *mut c_void,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}

pub fn owned_texture_descriptor_to_native(
    fields: TextureDescriptorFields,
) -> sys::mln_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_owned_texture_descriptor_default() };
    raw.width = fields.width;
    raw.height = fields.height;
    raw.scale_factor = fields.scale_factor;
    raw
}

pub fn metal_surface_descriptor_to_native(
    fields: MetalSurfaceDescriptorFields,
) -> sys::mln_metal_surface_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_metal_surface_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.layer = fields.layer;
    raw.device = fields.device;
    raw
}

pub fn vulkan_surface_descriptor_to_native(
    fields: VulkanSurfaceDescriptorFields,
) -> sys::mln_vulkan_surface_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.instance = fields.instance;
    raw.physical_device = fields.physical_device;
    raw.device = fields.device;
    raw.graphics_queue = fields.graphics_queue;
    raw.graphics_queue_family_index = fields.graphics_queue_family_index;
    raw.surface = fields.surface;
    raw
}

pub fn metal_owned_texture_descriptor_to_native(
    fields: MetalOwnedTextureDescriptorFields,
) -> sys::mln_metal_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.device = fields.device;
    raw
}

pub fn metal_borrowed_texture_descriptor_to_native(
    fields: MetalBorrowedTextureDescriptorFields,
) -> sys::mln_metal_borrowed_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.texture = fields.texture_handle;
    raw
}

pub fn vulkan_owned_texture_descriptor_to_native(
    fields: VulkanOwnedTextureDescriptorFields,
) -> sys::mln_vulkan_owned_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.instance = fields.instance;
    raw.physical_device = fields.physical_device;
    raw.device = fields.device;
    raw.graphics_queue = fields.graphics_queue;
    raw.graphics_queue_family_index = fields.graphics_queue_family_index;
    raw
}

pub fn vulkan_borrowed_texture_descriptor_to_native(
    fields: VulkanBorrowedTextureDescriptorFields,
) -> sys::mln_vulkan_borrowed_texture_descriptor {
    // SAFETY: Default constructor takes no arguments and initializes size.
    let mut raw = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    raw.width = fields.texture.width;
    raw.height = fields.texture.height;
    raw.scale_factor = fields.texture.scale_factor;
    raw.instance = fields.instance;
    raw.physical_device = fields.physical_device;
    raw.device = fields.device;
    raw.graphics_queue = fields.graphics_queue;
    raw.graphics_queue_family_index = fields.graphics_queue_family_index;
    raw.image = fields.image;
    raw.image_view = fields.image_view;
    raw.format = fields.format;
    raw.initial_layout = fields.initial_layout;
    raw.final_layout = fields.final_layout;
    raw
}

#[cfg(test)]
mod tests {
    use super::*;

    fn ptr(address: usize) -> *mut c_void {
        address as *mut c_void
    }

    fn texture() -> TextureDescriptorFields {
        TextureDescriptorFields {
            width: 32,
            height: 16,
            scale_factor: 2.0,
        }
    }

    #[test]
    fn owned_and_metal_descriptors_fill_sizes_fields_and_pointers() {
        let owned = owned_texture_descriptor_to_native(texture());
        assert_eq!(
            owned.size,
            std::mem::size_of::<sys::mln_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned.width, 32);
        assert_eq!(owned.height, 16);
        assert_eq!(owned.scale_factor, 2.0);

        let surface = metal_surface_descriptor_to_native(MetalSurfaceDescriptorFields {
            texture: texture(),
            layer: ptr(1),
            device: ptr(2),
        });
        assert_eq!(
            surface.size,
            std::mem::size_of::<sys::mln_metal_surface_descriptor>() as u32
        );
        assert_eq!(surface.layer, ptr(1));
        assert_eq!(surface.device, ptr(2));

        let owned_texture =
            metal_owned_texture_descriptor_to_native(MetalOwnedTextureDescriptorFields {
                texture: texture(),
                device: ptr(3),
            });
        assert_eq!(
            owned_texture.size,
            std::mem::size_of::<sys::mln_metal_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned_texture.device, ptr(3));

        let borrowed =
            metal_borrowed_texture_descriptor_to_native(MetalBorrowedTextureDescriptorFields {
                texture: texture(),
                texture_handle: ptr(4),
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
            texture: texture(),
            instance: ptr(1),
            physical_device: ptr(2),
            device: ptr(3),
            graphics_queue: ptr(4),
            graphics_queue_family_index: 5,
            surface: ptr(6),
        });
        assert_eq!(
            surface.size,
            std::mem::size_of::<sys::mln_vulkan_surface_descriptor>() as u32
        );
        assert_eq!(surface.instance, ptr(1));
        assert_eq!(surface.graphics_queue_family_index, 5);
        assert_eq!(surface.surface, ptr(6));

        let owned = vulkan_owned_texture_descriptor_to_native(VulkanOwnedTextureDescriptorFields {
            texture: texture(),
            instance: ptr(7),
            physical_device: ptr(8),
            device: ptr(9),
            graphics_queue: ptr(10),
            graphics_queue_family_index: 11,
        });
        assert_eq!(
            owned.size,
            std::mem::size_of::<sys::mln_vulkan_owned_texture_descriptor>() as u32
        );
        assert_eq!(owned.instance, ptr(7));
        assert_eq!(owned.graphics_queue_family_index, 11);

        let borrowed =
            vulkan_borrowed_texture_descriptor_to_native(VulkanBorrowedTextureDescriptorFields {
                texture: texture(),
                instance: ptr(12),
                physical_device: ptr(13),
                device: ptr(14),
                graphics_queue: ptr(15),
                graphics_queue_family_index: 16,
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
}
