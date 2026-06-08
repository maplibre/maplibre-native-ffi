use std::error::Error;

use winit::window::Window;

#[cfg(target_os = "macos")]
use crate::metal::MetalContext;
use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;

pub enum GraphicsContext {
    #[cfg(target_os = "macos")]
    Metal(MetalContext),
    Vulkan(Box<VulkanContext>),
}

impl GraphicsContext {
    pub fn new(
        window: &Window,
        backends: maplibre_native::RenderBackendMask,
    ) -> Result<Self, Box<dyn Error>> {
        #[cfg(target_os = "macos")]
        if backends.contains(maplibre_native::RenderBackendMask::METAL) {
            return Ok(Self::Metal(MetalContext::new(window)?));
        }
        if backends.contains(maplibre_native::RenderBackendMask::VULKAN) {
            return Ok(Self::Vulkan(Box::new(VulkanContext::new(window)?)));
        }
        Err("no usable graphics backend is supported by the loaded native library".into())
    }

    pub fn backend_name(&self) -> &'static str {
        match self {
            #[cfg(target_os = "macos")]
            Self::Metal(_) => "Metal",
            Self::Vulkan(_) => "Vulkan",
        }
    }

    pub fn wait_idle(&self) -> Result<(), Box<dyn Error>> {
        match self {
            #[cfg(target_os = "macos")]
            Self::Metal(context) => {
                context.wait_idle();
                Ok(())
            }
            Self::Vulkan(context) => context.wait_idle().map_err(Into::into),
        }
    }

    #[cfg(target_os = "macos")]
    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Metal(context) => {
                context.resize(viewport);
                Ok(())
            }
            Self::Vulkan(_) => Ok(()),
        }
    }

    #[cfg(not(target_os = "macos"))]
    pub fn resize(&self, _viewport: Viewport) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Vulkan(_) => Ok(()),
        }
    }
}
