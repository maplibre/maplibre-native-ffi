use std::error::Error as StdError;

use maplibre_native_ffi::{
    GpuSync, MapHandle, OpenglBorrowedTextureDescriptor, OpenglContextDescriptor,
    OpenglOwnedTextureDescriptor, OpenglSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::opengl::{OpenGLBorrowedTexture, OpenGLContext, OpenGLTextureCompositor};
use crate::render_target::{
    Mode, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<OpenGLTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<OpenGLTextureCompositor>,
        /// The texture the session renders into as far as completed
        /// replacements show.
        texture: Box<OpenGLBorrowedTexture>,
        replacements: Replacements<OpenGLBorrowedTexture>,
    },
    Surface {
        session: Session,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        graphics: &GraphicsContext,
        viewport: Viewport,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<Self> {
        let gl = graphics.opengl();
        let context = gl.descriptor().map_err(|error| {
            compositor_error(format!("OpenGL context descriptor failed: {error}"))
        })?;
        let options = attach_options(wakes, mode);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = OpenglOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context,
                };
                let session = Session::new(
                    unsafe { map.opengl_owned_texture_attach(&descriptor, &options) }?,
                    false,
                )?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor(gl, viewport)?),
                })
            }
            Mode::BorrowedTexture => {
                let texture = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let descriptor = borrowed_descriptor(context, &texture, viewport);
                let session = Session::new(
                    unsafe { map.opengl_borrowed_texture_attach(&descriptor, &options) }?,
                    false,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(gl, viewport)?),
                    texture: Box::new(texture),
                    replacements: Replacements::default(),
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
                    session: Session::new(
                        unsafe { map.opengl_surface_attach(&descriptor, &options) }?,
                        true,
                    )?,
                })
            }
        }
    }

    pub fn session_mut(&mut self) -> &mut Session {
        match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session,
        }
    }

    /// Starts the session resize or target replacement a new viewport needs.
    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        map: &MapState,
        viewport: Viewport,
    ) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport);
                session.resize(viewport)?;
                Ok(())
            }
            Self::BorrowedTexture {
                session,
                compositor,
                replacements,
                ..
            } => {
                let gl = graphics.opengl();
                let replacement = OpenGLBorrowedTexture::new(gl, viewport).map_err(|error| {
                    compositor_error(format!("OpenGL texture creation failed: {error}"))
                })?;
                let context = gl
                    .descriptor()
                    .map_err(|error| compositor_error(error.to_string()))?;
                let descriptor = borrowed_descriptor(context, &replacement, viewport);
                let completion = unsafe {
                    session
                        .handle()
                        .opengl_borrowed_texture_set_target(&descriptor)
                }?;
                replacements.push(completion, replacement);
                compositor.resize(viewport);
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::Surface { session } => {
                session.resize(viewport)?;
                Ok(())
            }
        }
    }

    /// Services caller-driver work, then releases what completed target
    /// replacements retired.
    pub fn service(&mut self, graphics: &GraphicsContext) -> maplibre_native_ffi::Result<()> {
        self.session_mut().service()?;
        if let Self::BorrowedTexture {
            texture,
            replacements,
            ..
        } = self
        {
            while let Some(replacement) = replacements.take_completed()? {
                std::mem::replace(&mut **texture, replacement).close(Some(graphics.opengl()));
            }
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(&mut self, graphics: &GraphicsContext) -> maplibre_native_ffi::Result<bool> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                // Without a new frame, the window keeps the one it already
                // shows.
                let Some(frame) = session.acquire_newest()? else {
                    return Ok(true);
                };
                require_cpu_complete_producer(&frame)?;
                let drawn = compositor.draw_frame(graphics.opengl(), &frame);
                frame.release(&GpuSync::default())?;
                drawn?;
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => compositor.draw_texture(graphics.opengl(), texture.texture())?,
            // The driver already presented the frame.
            Self::Surface { .. } => {}
        }
        Ok(true)
    }

    pub fn close(self, graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        let gl = Some(graphics.opengl());
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                session.close()?;
                compositor.close(gl);
                Ok(())
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
                mut replacements,
            } => {
                session.close()?;
                for replacement in replacements.take_all() {
                    replacement.close(gl);
                }
                compositor.close(gl);
                texture.close(gl);
                Ok(())
            }
            Self::Surface { session } => session.close(),
        }
    }
}

fn compositor(
    gl: &OpenGLContext,
    viewport: Viewport,
) -> maplibre_native_ffi::Result<OpenGLTextureCompositor> {
    OpenGLTextureCompositor::new(gl, viewport)
        .map_err(|error| compositor_error(format!("OpenGL compositor creation failed: {error}")))
}

fn borrowed_descriptor(
    context: OpenglContextDescriptor,
    texture: &OpenGLBorrowedTexture,
    viewport: Viewport,
) -> OpenglBorrowedTextureDescriptor {
    OpenglBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        context,
        texture: texture.texture(),
        target: texture.target(),
    }
}
