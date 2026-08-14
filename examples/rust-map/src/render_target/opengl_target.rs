use std::error::Error as StdError;

use maplibre_native_ffi::{
    Error, GpuSync, MapHandle, OpenGLBorrowedTextureDescriptor, OpenGLOwnedTextureDescriptor,
    OpenGLSurfaceDescriptor, OperationHandle, RenderSessionAttachOptions, RenderSessionAttachment,
    RenderSessionHandle,
};

use crate::graphics::GraphicsContext;
use crate::opengl::{OpenGLBorrowedTexture, OpenGLTextureCompositor};
use crate::render_target::{Mode, extent, request_render_frame, require_cpu_complete_producer};
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<OpenGLTextureCompositor>,
    },
    BorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<OpenGLTextureCompositor>,
        texture: Box<OpenGLBorrowedTexture>,
    },
    Surface {
        session: RenderSessionHandle,
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
        let options =
            RenderSessionAttachOptions::caller_graphics_thread(if mode == Mode::OwnedTexture {
                2
            } else {
                0
            });
        match mode {
            Mode::OwnedTexture => {
                let descriptor = OpenGLOwnedTextureDescriptor::new(extent(viewport), context);
                let attachment = map.attach_opengl_owned_texture(&descriptor, options)?;
                let session = finish_attach(attachment)?;
                let compositor = OpenGLTextureCompositor::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL compositor creation failed: {error}"))
                })?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let texture = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let descriptor = OpenGLBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    context,
                    texture.texture(),
                    texture.target(),
                );
                let session =
                    finish_attach(map.attach_opengl_borrowed_texture(&descriptor, options)?)?;
                let compositor = OpenGLTextureCompositor::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL compositor creation failed: {error}"))
                })?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor),
                    texture: Box::new(texture),
                })
            }
            Mode::NativeSurface => {
                let surface = gl.surface_pointer().map_err(|error| {
                    compositor_error(format!("OpenGL surface handle failed: {error}"))
                })?;
                let descriptor = OpenGLSurfaceDescriptor::new(extent(viewport), context, surface);
                let session = finish_attach(map.attach_opengl_surface(&descriptor, options)?)?;
                Ok(Self::Surface { session })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<()> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport);
                session.resize(&extent(viewport))?.release();
                Ok(())
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                let gl = graphics.opengl();
                let replacement = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let descriptor = OpenGLBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    gl.descriptor()
                        .map_err(|error| compositor_error(error.to_string()))?,
                    replacement.texture(),
                    replacement.target(),
                );
                let operation = session.set_opengl_borrowed_texture_target(&descriptor)?;
                drive(session, &operation)?;
                operation.release();
                compositor.resize(viewport);
                let outgoing = std::mem::replace(&mut **texture, replacement);
                outgoing.close(Some(gl));
                Ok(())
            }
            Self::Surface { session } => {
                session.resize(&extent(viewport))?.release();
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<bool> {
        let present = matches!(self, Self::Surface { .. });
        let session = match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session,
        };
        let rendered = request_render_frame(session, present, true)?;
        if !rendered {
            return Ok(false);
        }
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                let frame = session.acquire_frame()?;
                require_cpu_complete_producer(&frame)?;
                compositor.draw_frame(graphics.opengl(), &frame)?;
                frame
                    .release(GpuSync::CPU_COMPLETE)
                    .map_err(|error| error.into_error())?
                    .release();
                Ok(true)
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => {
                compositor.draw_texture(graphics.opengl(), texture.texture())?;
                Ok(true)
            }
            Self::Surface { .. } => Ok(true),
        }
    }

    pub fn close(self, graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                detach(&session)?;
                compositor.close(Some(graphics.opengl()));
                destroy(session)
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                detach(&session)?;
                compositor.close(Some(graphics.opengl()));
                texture.close(Some(graphics.opengl()));
                destroy(session)
            }
            Self::Surface { session } => {
                detach(&session)?;
                destroy(session)
            }
        }
    }
}

fn finish_attach(
    attachment: RenderSessionAttachment,
) -> maplibre_native_ffi::Result<RenderSessionHandle> {
    drive(&attachment.session, &attachment.operation)?;
    let session = attachment.session;
    attachment.operation.release();
    Ok(session)
}

fn drive(
    session: &RenderSessionHandle,
    operation: &OperationHandle<()>,
) -> maplibre_native_ffi::Result<()> {
    while !operation.is_completed()? {
        if session.service_driver_work(usize::MAX)? == 0 {
            std::thread::yield_now();
        }
    }
    if operation.terminal_status()? != 0 {
        return Err(compositor_error(operation.diagnostic()?));
    }
    Ok(())
}

fn detach(session: &RenderSessionHandle) -> Result<(), Box<dyn StdError>> {
    let operation = session.detach()?;
    drive(session, &operation)?;
    operation.release();
    Ok(())
}

fn destroy(session: RenderSessionHandle) -> Result<(), Box<dyn StdError>> {
    session
        .destroy()
        .map_err(|error| Box::new(error) as Box<dyn StdError>)
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(maplibre_native_ffi::ErrorKind::NativeError, None, message)
}
