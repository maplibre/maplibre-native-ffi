use std::error::Error;

use winit::event_loop::ActiveEventLoop;
use winit::window::Window;
use winit::window::WindowAttributes;

#[cfg(target_os = "macos")]
use crate::metal::MetalContext;
#[cfg(any(target_os = "linux", target_os = "windows"))]
use crate::opengl::OpenGLContext;
use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;

pub enum GraphicsContext {
    #[cfg(target_os = "macos")]
    Metal(MetalContext),
    Vulkan(Box<VulkanContext>),
    #[cfg(any(target_os = "linux", target_os = "windows"))]
    OpenGL(Box<OpenGLContext>),
}

impl GraphicsContext {
    pub fn create_window(
        event_loop: &ActiveEventLoop,
        window_attributes: WindowAttributes,
        backends: maplibre_native::RenderBackendMask,
    ) -> Result<(Window, Self), Box<dyn Error>> {
        #[cfg(target_os = "macos")]
        if backends.contains(maplibre_native::RenderBackendMask::METAL) {
            let window = event_loop.create_window(window_attributes.clone())?;
            let context = MetalContext::new(&window)?;
            return Ok((window, Self::Metal(context)));
        }
        #[cfg(any(target_os = "linux", target_os = "windows"))]
        if backends.contains(maplibre_native::RenderBackendMask::OPENGL) {
            let (window, context) = OpenGLContext::new(event_loop, window_attributes.clone())?;
            return Ok((window, Self::OpenGL(Box::new(context))));
        }
        if backends.contains(maplibre_native::RenderBackendMask::VULKAN) {
            let window = event_loop.create_window(window_attributes)?;
            let context = VulkanContext::new(&window)?;
            return Ok((window, Self::Vulkan(Box::new(context))));
        }
        Err("no usable graphics backend is supported by the loaded native library".into())
    }

    pub fn backend_name(&self) -> &'static str {
        match self {
            #[cfg(target_os = "macos")]
            Self::Metal(_) => "Metal",
            Self::Vulkan(_) => "Vulkan",
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(_) => "OpenGL",
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
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(context) => {
                context.wait_idle();
                Ok(())
            }
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
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(context) => context.resize(_viewport),
        }
    }
}
