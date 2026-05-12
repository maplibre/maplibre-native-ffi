use maplibre_native::{MapHandle, RenderSessionHandle, VulkanSurfaceDescriptor};

use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;

pub enum Mode {
    NativeSurface,
}

pub struct RenderTarget {
    session: RenderSessionHandle,
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        match mode {
            Mode::NativeSurface => Self::attach_vulkan_surface(map, vulkan, viewport),
        }
    }

    pub fn resize(&self, viewport: Viewport) -> maplibre_native::Result<()> {
        self.session.resize(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        )
    }

    pub fn render_update(&self) -> maplibre_native::Result<()> {
        self.session.render_update()
    }

    pub fn close(&self) -> maplibre_native::Result<()> {
        self.session.close()
    }

    fn attach_vulkan_surface(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanSurfaceDescriptor::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
            vulkan.instance_pointer(),
            vulkan.physical_device_pointer(),
            vulkan.device_pointer(),
            vulkan.graphics_queue_pointer(),
            vulkan.graphics_queue_family_index(),
            vulkan.surface_pointer(),
        );
        Ok(Self {
            session: map.attach_vulkan_surface(&descriptor)?,
        })
    }
}
