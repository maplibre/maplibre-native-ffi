use std::error::Error as StdError;

use maplibre_native_ffi::{
    Error, MapAttachRef, OpenGLBorrowedTextureDescriptor, OpenGLOwnedTextureDescriptor,
    OpenGLSurfaceDescriptor, RenderResult, RenderSessionHandle,
};

use crate::graphics::GraphicsContext;
use crate::opengl::{OpenGLBorrowedTexture, OpenGLContext, OpenGLTextureCompositor};
use crate::render_target::{Mode, extent};
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
        map: &MapAttachRef,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let opengl = graphics.opengl();
        match mode {
            Mode::OwnedTexture => Self::attach_owned_texture(map, opengl, viewport),
            Mode::BorrowedTexture => Self::attach_borrowed_texture(map, opengl, viewport),
            Mode::NativeSurface => Self::attach_surface(map, opengl, viewport),
        }
    }

    pub fn attach_owned_texture(
        map: &MapAttachRef,
        opengl: &OpenGLContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let context = opengl.descriptor().map_err(|error| {
            compositor_error(format!("OpenGL context descriptor failed: {error}"))
        })?;
        let descriptor = OpenGLOwnedTextureDescriptor::new(extent(viewport), context);
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
        Ok(Self::OwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    pub fn attach_borrowed_texture(
        map: &MapAttachRef,
        opengl: &OpenGLContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let context = opengl.descriptor().map_err(|error| {
            compositor_error(format!("OpenGL context descriptor failed: {error}"))
        })?;
        let texture = OpenGLBorrowedTexture::new(opengl, viewport).map_err(|error| {
            compositor_error(format!("OpenGL borrowed texture creation failed: {error}"))
        })?;
        let descriptor = OpenGLBorrowedTextureDescriptor::new(
            extent(viewport),
            viewport.physical_width,
            viewport.physical_height,
            context,
            texture.texture(),
            texture.target(),
        );
        let session = match map.attach_opengl_borrowed_texture(&descriptor) {
            Ok(session) => session,
            Err(error) => {
                texture.close(Some(opengl));
                return Err(error);
            }
        };
        let compositor = match OpenGLTextureCompositor::new(opengl, viewport) {
            Ok(compositor) => compositor,
            Err(error) => {
                let mut message = format!("OpenGL texture compositor creation failed: {error}");
                if let Err(close_error) = session.close() {
                    message.push_str(&format!("; render session cleanup failed: {close_error}"));
                }
                texture.close(Some(opengl));
                return Err(compositor_error(message));
            }
        };
        Ok(Self::BorrowedTexture {
            session,
            compositor: Box::new(compositor),
            texture: Box::new(texture),
        })
    }

    pub fn attach_surface(
        map: &MapAttachRef,
        opengl: &OpenGLContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let context = opengl.descriptor().map_err(|error| {
            compositor_error(format!("OpenGL context descriptor failed: {error}"))
        })?;
        let surface = opengl
            .surface_pointer()
            .map_err(|error| compositor_error(format!("OpenGL surface handle failed: {error}")))?;
        let descriptor = OpenGLSurfaceDescriptor::new(extent(viewport), context, surface);
        Ok(Self::Surface {
            session: map.attach_opengl_surface(&descriptor)?,
        })
    }

    /// Resizes without closing the session; a caller-owned texture is replaced
    /// with one at the new size and handed over.
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
                session.resize(
                    viewport.logical_width,
                    viewport.logical_height,
                    viewport.scale_factor,
                )
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                let opengl = graphics.opengl();
                let replacement =
                    OpenGLBorrowedTexture::new(opengl, viewport).map_err(|error| {
                        compositor_error(format!(
                            "OpenGL borrowed texture creation failed: {error}"
                        ))
                    })?;
                let context = opengl.descriptor().map_err(|error| {
                    compositor_error(format!("OpenGL context descriptor failed: {error}"))
                })?;
                let descriptor = OpenGLBorrowedTextureDescriptor::new(
                    extent(viewport),
                    viewport.physical_width,
                    viewport.physical_height,
                    context,
                    replacement.texture(),
                    replacement.target(),
                );
                // On failure the session may already hold the replacement, so
                // leak it rather than hand the session a dangling texture.
                session.set_opengl_borrowed_texture_target(&descriptor)?;
                compositor.resize(viewport);
                let outgoing = std::mem::replace(&mut **texture, replacement);
                outgoing.close(Some(opengl));
                Ok(())
            }
            Self::Surface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(
        &mut self,
        graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<bool> {
        let opengl = graphics.opengl();
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                if session.render_update()?.result != RenderResult::Rendered {
                    return Ok(false);
                }
                let frame = session.acquire_opengl_owned_texture_frame()?;
                let draw_result = compositor.draw_frame(opengl, &frame);
                let close_result = frame.close().map_err(|error| error.into_error());
                match (draw_result, close_result) {
                    (Ok(()), Ok(())) => Ok(true),
                    (Err(draw_error), Ok(())) => Err(draw_error),
                    (Ok(()), Err(close_error)) => Err(close_error),
                    (Err(draw_error), Err(close_error)) => Err(Error::new(
                        draw_error.kind(),
                        draw_error.raw_status(),
                        format!("{draw_error}; frame cleanup failed: {close_error}"),
                    )),
                }
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                if session.render_update()?.result != RenderResult::Rendered {
                    return Ok(false);
                }
                compositor.draw_texture(opengl, texture.texture())?;
                Ok(true)
            }
            Self::Surface { session } => {
                Ok(session.render_update()?.result == RenderResult::Rendered)
            }
        }
    }

    pub fn close(self, graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        let opengl = Some(graphics.opengl());
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.close(opengl);
                session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                compositor.close(opengl);
                let result = session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>);
                texture.close(opengl);
                result
            }
            Self::Surface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
        }
    }
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(maplibre_native_ffi::ErrorKind::NativeError, None, message)
}
