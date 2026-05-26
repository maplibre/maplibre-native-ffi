use maplibre_native::{
    Error, ErrorKind, MapHandle, OpenGLContextDescriptor, OpenGLOwnedTextureDescriptor,
    OpenGLSurfaceDescriptor, RenderBackendMask, RenderSessionHandle, RenderTargetExtent,
    VulkanContextDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};
use std::error::Error as StdError;
use winit::window::Window;

use crate::opengl::OpenGLContext;
use crate::opengl_texture_compositor::OpenGLTextureCompositor;
use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Backend {
    Vulkan,
    OpenGL,
}

impl Backend {
    pub fn cli_name(self) -> &'static str {
        match self {
            Self::Vulkan => "vulkan",
            Self::OpenGL => "opengl",
        }
    }

    pub fn parse(value: &str) -> Result<Self, String> {
        match value {
            "vulkan" => Ok(Self::Vulkan),
            "opengl" => Ok(Self::OpenGL),
            _ => Err(format!("unknown backend '{value}'")),
        }
    }

    pub fn choose_default() -> Self {
        let backends = maplibre_native::supported_render_backends();
        if backends.contains(RenderBackendMask::VULKAN) {
            return Self::Vulkan;
        }
        if backends.contains(RenderBackendMask::OPENGL) {
            return Self::OpenGL;
        }
        Self::Vulkan
    }

    pub fn ensure_supported(self) -> Result<(), Box<dyn StdError>> {
        let backends = maplibre_native::supported_render_backends();
        match self {
            Self::Vulkan if backends.contains(RenderBackendMask::VULKAN) => Ok(()),
            Self::Vulkan => {
                Err("the loaded MapLibre native library does not support Vulkan".into())
            }
            Self::OpenGL if backends.contains(RenderBackendMask::OPENGL) => Ok(()),
            Self::OpenGL => {
                Err("the loaded MapLibre native library does not support OpenGL".into())
            }
        }
    }
}

pub enum BackendContext {
    Vulkan(Box<VulkanContext>),
    OpenGL(OpenGLContext),
}

impl BackendContext {
    pub fn new(backend: Backend, window: &Window) -> Result<Self, Box<dyn StdError>> {
        match backend {
            Backend::Vulkan => Ok(Self::Vulkan(Box::new(VulkanContext::new(window)?))),
            Backend::OpenGL => Ok(Self::OpenGL(OpenGLContext::new(window)?)),
        }
    }

    pub fn wait_idle(&self) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::Vulkan(vulkan) => vulkan
                .wait_idle()
                .map_err(|error| format!("{error:?}").into()),
            Self::OpenGL(opengl) => opengl.wait_idle(),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Mode {
    OwnedTexture,
    NativeSurface,
}

impl Mode {
    pub fn cli_name(self) -> &'static str {
        match self {
            Self::OwnedTexture => "owned-texture",
            Self::NativeSurface => "native-surface",
        }
    }

    pub fn status(self) -> &'static str {
        match self {
            Self::OwnedTexture => "samples MapLibre-owned frames into the winit swapchain",
            Self::NativeSurface => "renders directly to the winit native surface",
        }
    }

    pub fn parse(value: &str) -> Result<Self, String> {
        match value {
            "owned-texture" => Ok(Self::OwnedTexture),
            "native-surface" => Ok(Self::NativeSurface),
            _ => Err(format!("unknown render target '{value}'")),
        }
    }
}

pub enum RenderTarget {
    VulkanOwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
    },
    VulkanNativeSurface {
        session: RenderSessionHandle,
    },
    OpenGLOwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<OpenGLTextureCompositor>,
    },
    OpenGLNativeSurface {
        session: RenderSessionHandle,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        backend: &BackendContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        match (backend, mode) {
            (BackendContext::Vulkan(vulkan), Mode::OwnedTexture) => {
                Self::attach_vulkan_owned_texture(map, vulkan, viewport)
            }
            (BackendContext::Vulkan(vulkan), Mode::NativeSurface) => {
                Self::attach_vulkan_surface(map, vulkan, viewport)
            }
            (BackendContext::OpenGL(opengl), Mode::OwnedTexture) => {
                Self::attach_opengl_owned_texture(map, opengl, viewport)
            }
            (BackendContext::OpenGL(opengl), Mode::NativeSurface) => {
                Self::attach_opengl_surface(map, opengl, viewport)
            }
        }
    }

    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native::Result<()> {
        match self {
            Self::VulkanOwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!(
                        "Vulkan texture compositor resize failed: {error:?}"
                    ))
                })?;
                session.resize(
                    viewport.logical_width,
                    viewport.logical_height,
                    viewport.scale_factor,
                )
            }
            Self::VulkanNativeSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            Self::OpenGLOwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport);
                session.resize(
                    viewport.logical_width,
                    viewport.logical_height,
                    viewport.scale_factor,
                )
            }
            Self::OpenGLNativeSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(&mut self) -> maplibre_native::Result<()> {
        match self {
            Self::VulkanOwnedTexture {
                session,
                compositor,
            } => {
                session.render_update()?;
                let frame = session.acquire_vulkan_owned_texture_frame()?;
                let draw_result = compositor.draw(&frame);
                let close_result = frame.close().map_err(|error| error.into_error());
                match (draw_result, close_result) {
                    (Ok(()), Ok(())) => Ok(()),
                    (Err(draw_error), Ok(())) => Err(draw_error),
                    (Ok(()), Err(close_error)) => Err(close_error),
                    (Err(draw_error), Err(close_error)) => Err(Error::new(
                        draw_error.kind(),
                        draw_error.raw_status(),
                        format!("{draw_error}; frame cleanup failed: {close_error}"),
                    )),
                }
            }
            Self::VulkanNativeSurface { session } => session.render_update(),
            Self::OpenGLOwnedTexture {
                session,
                compositor,
            } => {
                session.render_update()?;
                let frame = session.acquire_opengl_owned_texture_frame()?;
                let draw_result = compositor.draw(&frame);
                let close_result = frame.close().map_err(|error| error.into_error());
                match (draw_result, close_result) {
                    (Ok(()), Ok(())) => Ok(()),
                    (Err(draw_error), Ok(())) => Err(draw_error),
                    (Ok(()), Err(close_error)) => Err(close_error),
                    (Err(draw_error), Err(close_error)) => Err(Error::new(
                        draw_error.kind(),
                        draw_error.raw_status(),
                        format!("{draw_error}; frame cleanup failed: {close_error}"),
                    )),
                }
            }
            Self::OpenGLNativeSurface { session } => session.render_update(),
        }
    }

    pub fn close(self) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::VulkanOwnedTexture {
                session,
                mut compositor,
            } => {
                let mut close_error = compositor
                    .close()
                    .err()
                    .map(|error| format!("Vulkan texture compositor close failed: {error:?}"));
                if let Err(error) = session.close() {
                    append_error(
                        &mut close_error,
                        format!("render session close failed: {error}"),
                    );
                }
                match close_error {
                    Some(error) => Err(Box::new(compositor_error(error))),
                    None => Ok(()),
                }
            }
            Self::VulkanNativeSurface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
            Self::OpenGLOwnedTexture {
                session,
                mut compositor,
            } => {
                let mut close_error = compositor
                    .close()
                    .err()
                    .map(|error| format!("OpenGL texture compositor close failed: {error}"));
                if let Err(error) = session.close() {
                    append_error(
                        &mut close_error,
                        format!("render session close failed: {error}"),
                    );
                }
                match close_error {
                    Some(error) => Err(Box::new(compositor_error(error))),
                    None => Ok(()),
                }
            }
            Self::OpenGLNativeSurface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
        }
    }

    fn attach_vulkan_owned_texture(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanOwnedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            VulkanContextDescriptor::new(
                vulkan.instance_pointer(),
                vulkan.physical_device_pointer(),
                vulkan.device_pointer(),
                vulkan.graphics_queue_pointer(),
                vulkan.graphics_queue_family_index(),
            )
            .with_proc_addresses(
                vulkan.get_instance_proc_addr_pointer(),
                vulkan.get_device_proc_addr_pointer(),
            ),
        );
        let session = map.attach_vulkan_owned_texture(&descriptor)?;
        let compositor = match VulkanTextureCompositor::new(vulkan, viewport) {
            Ok(compositor) => compositor,
            Err(error) => {
                let mut message = format!("Vulkan texture compositor creation failed: {error:?}");
                if let Err(close_error) = session.close() {
                    message.push_str(&format!("; render session cleanup failed: {close_error}"));
                }
                return Err(compositor_error(message));
            }
        };
        Ok(Self::VulkanOwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    fn attach_vulkan_surface(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanSurfaceDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            VulkanContextDescriptor::new(
                vulkan.instance_pointer(),
                vulkan.physical_device_pointer(),
                vulkan.device_pointer(),
                vulkan.graphics_queue_pointer(),
                vulkan.graphics_queue_family_index(),
            )
            .with_proc_addresses(
                vulkan.get_instance_proc_addr_pointer(),
                vulkan.get_device_proc_addr_pointer(),
            ),
            vulkan.surface_pointer(),
        );
        Ok(Self::VulkanNativeSurface {
            session: map.attach_vulkan_surface(&descriptor)?,
        })
    }

    fn attach_opengl_owned_texture(
        map: &MapHandle,
        opengl: &OpenGLContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            opengl_context_descriptor(opengl),
        );
        let session = map.attach_opengl_owned_texture(&descriptor)?;
        let compositor = match OpenGLTextureCompositor::new(opengl, viewport) {
            Ok(compositor) => compositor,
            Err(error) => {
                let mut message = format!("OpenGL texture compositor creation failed: {error}");
                if let Err(close_error) = session.close() {
                    message.push_str(&format!("; render session cleanup failed: {close_error}"));
                }
                return Err(compositor_error(message));
            }
        };
        Ok(Self::OpenGLOwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    fn attach_opengl_surface(
        map: &MapHandle,
        opengl: &OpenGLContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = OpenGLSurfaceDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            opengl_context_descriptor(opengl),
            opengl.surface_pointer(),
        );
        Ok(Self::OpenGLNativeSurface {
            session: map.attach_opengl_surface(&descriptor)?,
        })
    }
}

fn opengl_context_descriptor(opengl: &OpenGLContext) -> OpenGLContextDescriptor {
    opengl.descriptor()
}

fn append_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => message.push_str(&format!("; {error}")),
        None => *message = Some(error),
    }
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
