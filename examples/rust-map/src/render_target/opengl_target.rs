use std::error::Error as StdError;

use maplibre_native_ffi::{
    GpuSync, MapHandle, OpenglBorrowedTextureDescriptor, OpenglOwnedTextureDescriptor,
    OpenglSurfaceDescriptor, RenderSessionAttachOptions,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::opengl::{OpenGLBorrowedTexture, OpenGLTextureCompositor};
use crate::render_target::{
    FrameDriver, FrameOutcome, Mode, compositor_error, extent, require_cpu_complete_producer,
};
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        driver: FrameDriver,
        compositor: Box<OpenGLTextureCompositor>,
    },
    BorrowedTexture {
        driver: FrameDriver,
        compositor: Box<OpenGLTextureCompositor>,
        texture: Box<OpenGLBorrowedTexture>,
    },
    Surface {
        driver: FrameDriver,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let gl = graphics.opengl();
        let context = gl.descriptor().map_err(|error| {
            compositor_error(format!("OpenGL context descriptor failed: {error}"))
        })?;
        let options = RenderSessionAttachOptions {
            driver: maplibre_native_ffi::RenderDriverKind::CallerGraphicsThread,
            requested_texture_ring_depth: if mode == Mode::OwnedTexture { 2 } else { 0 },
            ..Default::default()
        };
        match mode {
            Mode::OwnedTexture => {
                let descriptor = OpenglOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context,
                };
                let driver = FrameDriver::new(unsafe {
                    map.opengl_owned_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = OpenGLTextureCompositor::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL compositor creation failed: {error}"))
                })?;
                Ok(Self::OwnedTexture {
                    driver,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let texture = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let descriptor = OpenglBorrowedTextureDescriptor {
                    extent: extent(viewport),
                    physical_width: viewport.physical_width,
                    physical_height: viewport.physical_height,
                    context,
                    texture: texture.texture(),
                    target: texture.target(),
                };
                let driver = FrameDriver::new(unsafe {
                    map.opengl_borrowed_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = OpenGLTextureCompositor::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL compositor creation failed: {error}"))
                })?;
                Ok(Self::BorrowedTexture {
                    driver,
                    compositor: Box::new(compositor),
                    texture: Box::new(texture),
                })
            }
            Mode::NativeSurface => {
                let surface = gl.surface_pointer().map_err(|error| {
                    compositor_error(format!("OpenGL surface handle failed: {error}"))
                })?;
                let descriptor = OpenglSurfaceDescriptor {
                    extent: extent(viewport),
                    context,
                    surface,
                };
                Ok(Self::Surface {
                    driver: FrameDriver::new(unsafe {
                        map.opengl_surface_attach(&descriptor, &(options))
                    }?)?,
                })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        map: &MapState,
        viewport: Viewport,
    ) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture { driver, compositor } => {
                compositor.resize(viewport);
                driver.resize(viewport)?;
                Ok(())
            }
            Self::BorrowedTexture {
                driver,
                compositor,
                texture,
            } => {
                let gl = graphics.opengl();
                let replacement = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let descriptor = OpenglBorrowedTextureDescriptor {
                    extent: extent(viewport),
                    physical_width: viewport.physical_width,
                    physical_height: viewport.physical_height,
                    context: gl
                        .descriptor()
                        .map_err(|error| compositor_error(error.to_string()))?,
                    texture: replacement.texture(),
                    target: replacement.target(),
                };
                let operation = unsafe {
                    driver
                        .session()
                        .opengl_borrowed_texture_set_target(&descriptor)
                }?;
                driver.drive(&operation)?;
                compositor.resize(viewport);
                let outgoing = std::mem::replace(&mut **texture, replacement);
                outgoing.close(Some(gl));
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::Surface { driver } => {
                driver.resize(viewport)?;
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<FrameOutcome> {
        let present = matches!(self, Self::Surface { .. });
        let driver = match self {
            Self::OwnedTexture { driver, .. }
            | Self::BorrowedTexture { driver, .. }
            | Self::Surface { driver } => driver,
        };
        let mut outcome = driver.render_frame(present)?;
        if !outcome.rendered {
            return Ok(outcome);
        }
        match self {
            Self::OwnedTexture { driver, compositor } => {
                let Some(frame) = driver.acquire_frame()? else {
                    outcome.rendered = false;
                    return Ok(outcome);
                };
                require_cpu_complete_producer(&frame)?;
                compositor.draw_frame(graphics.opengl(), &frame)?;
                frame.release(&GpuSync::default())?;
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => compositor.draw_texture(graphics.opengl(), texture.texture())?,
            Self::Surface { .. } => {}
        }
        Ok(outcome)
    }

    pub fn close(self, graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture { driver, compositor } => {
                driver.close()?;
                compositor.close(Some(graphics.opengl()));
                Ok(())
            }
            Self::BorrowedTexture {
                driver,
                compositor,
                texture,
            } => {
                driver.close()?;
                compositor.close(Some(graphics.opengl()));
                texture.close(Some(graphics.opengl()));
                Ok(())
            }
            Self::Surface { driver } => driver.close(),
        }
    }
}
