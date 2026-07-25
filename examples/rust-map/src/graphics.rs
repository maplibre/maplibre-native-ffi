use std::error::Error;

use maplibre_native::RenderBackendMask;
use winit::event_loop::ActiveEventLoop;
use winit::window::Window;
use winit::window::WindowAttributes;

#[cfg(maplibre_render_backend = "metal")]
use crate::metal::MetalContext;
#[cfg(maplibre_render_backend = "opengl")]
use crate::opengl::OpenGLContext;
use crate::viewport::Viewport;
#[cfg(maplibre_render_backend = "vulkan")]
use crate::vulkan::VulkanContext;

#[cfg(maplibre_render_backend = "metal")]
pub struct GraphicsContext(MetalContext);
#[cfg(maplibre_render_backend = "opengl")]
pub struct GraphicsContext(Box<OpenGLContext>);
#[cfg(maplibre_render_backend = "vulkan")]
pub struct GraphicsContext(Box<VulkanContext>);

pub fn required_backend() -> RenderBackendMask {
    #[cfg(maplibre_render_backend = "metal")]
    {
        RenderBackendMask::METAL
    }
    #[cfg(maplibre_render_backend = "opengl")]
    {
        RenderBackendMask::OPENGL
    }
    #[cfg(maplibre_render_backend = "vulkan")]
    {
        RenderBackendMask::VULKAN
    }
}

#[cfg(maplibre_render_backend = "metal")]
impl GraphicsContext {
    pub fn create_window(
        event_loop: &ActiveEventLoop,
        window_attributes: WindowAttributes,
        backends: RenderBackendMask,
    ) -> Result<(Window, Self), Box<dyn Error>> {
        if !backends.contains(required_backend()) {
            return Err(
                "no usable graphics backend is supported by the loaded native library".into(),
            );
        }
        let window = event_loop.create_window(window_attributes)?;
        let context = MetalContext::new(&window)?;
        Ok((window, Self(context)))
    }

    pub fn backend_name(&self) -> &'static str {
        "Metal"
    }

    pub fn wait_idle(&self) -> Result<(), Box<dyn Error>> {
        self.0.wait_idle();
        Ok(())
    }

    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.0.resize(viewport);
        Ok(())
    }

    pub fn metal(&self) -> &MetalContext {
        &self.0
    }
}

#[cfg(maplibre_render_backend = "opengl")]
impl GraphicsContext {
    pub fn create_window(
        event_loop: &ActiveEventLoop,
        window_attributes: WindowAttributes,
        backends: RenderBackendMask,
    ) -> Result<(Window, Self), Box<dyn Error>> {
        if !backends.contains(required_backend()) {
            return Err(
                "no usable graphics backend is supported by the loaded native library".into(),
            );
        }
        let (window, context) = OpenGLContext::new(event_loop, window_attributes)?;
        Ok((window, Self(Box::new(context))))
    }

    pub fn backend_name(&self) -> &'static str {
        "OpenGL"
    }

    pub fn wait_idle(&self) -> Result<(), Box<dyn Error>> {
        self.0.wait_idle();
        Ok(())
    }

    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.0.resize(viewport)
    }

    pub fn opengl(&self) -> &OpenGLContext {
        &self.0
    }
}

#[cfg(maplibre_render_backend = "vulkan")]
impl GraphicsContext {
    pub fn create_window(
        event_loop: &ActiveEventLoop,
        window_attributes: WindowAttributes,
        backends: RenderBackendMask,
    ) -> Result<(Window, Self), Box<dyn Error>> {
        if !backends.contains(required_backend()) {
            return Err(
                "no usable graphics backend is supported by the loaded native library".into(),
            );
        }
        let window = event_loop.create_window(window_attributes)?;
        let context = VulkanContext::new(&window)?;
        Ok((window, Self(Box::new(context))))
    }

    pub fn backend_name(&self) -> &'static str {
        "Vulkan"
    }

    pub fn wait_idle(&self) -> Result<(), Box<dyn Error>> {
        self.0.wait_idle().map_err(Into::into)
    }

    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        let _ = viewport;
        Ok(())
    }

    pub fn vulkan(&self) -> &VulkanContext {
        &self.0
    }
}
