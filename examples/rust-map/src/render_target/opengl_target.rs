use std::error::Error as StdError;

use maplibre_native_ffi::{
    MapHandle, OpenglBorrowedTexture, OpenglBorrowedTextureDescriptor, OpenglContextDescriptor,
    OpenglOwnedTextureDescriptor, OpenglSurfaceDescriptor, RenderDriverKind,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::opengl::{OpenGLBorrowedTexture, OpenGLContext, OpenGLTextureCompositor};
use crate::render_target::{
    Mode, RING_DEPTH, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;

/// The textures of a borrowed ring, one per slot.
type OpenGLRing = [OpenGLBorrowedTexture; RING_DEPTH];

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<OpenGLTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<OpenGLTextureCompositor>,
        /// The ring the session renders into.
        ring: Box<OpenGLRing>,
        replacements: Replacements<Box<OpenGLRing>>,
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
        // An OpenGL target on the host's WGL or EGL context renders where that
        // context is current, so the event loop services a caller driver.
        let options = attach_options(wakes, mode, RenderDriverKind::CallerGraphicsThread);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = OpenglOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context,
                };
                let session = Session::new(
                    unsafe { map.attach_opengl_owned_texture(&descriptor, &options) }?,
                    &options,
                    mode,
                    wakes,
                )?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor(gl, viewport)?),
                })
            }
            Mode::BorrowedTexture => {
                let ring = ring(gl, viewport)?;
                let descriptor = borrowed_descriptor(context, &ring, viewport);
                let session = Session::new(
                    unsafe { map.attach_opengl_borrowed_texture(&descriptor, &options) }?,
                    &options,
                    mode,
                    wakes,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(gl, viewport)?),
                    ring,
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
                        unsafe { map.attach_opengl_surface(&descriptor, &options) }?,
                        &options,
                        mode,
                        wakes,
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
        wakes: &Wakes,
    ) -> Result<(), Box<dyn StdError>> {
        graphics.opengl().resize(viewport)?;
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
                ring: current,
                replacements,
            } => {
                let gl = graphics.opengl();
                // A replacement is refused while the host holds a frame, and
                // the window keeps showing what it last presented.
                session.release_held()?;
                let replacement = ring(gl, viewport)?;
                let context = gl
                    .descriptor()
                    .map_err(|error| compositor_error(error.to_string()))?;
                let descriptor = borrowed_descriptor(context, &replacement, viewport);
                let completion = match unsafe {
                    session
                        .handle()
                        .set_opengl_borrowed_texture_target(&descriptor)
                } {
                    Ok(completion) => completion,
                    Err(error) => {
                        close_ring(replacement, Some(gl));
                        return Err(error.into());
                    }
                };
                let retired = std::mem::replace(current, replacement);
                replacements.push(completion, retired, wakes);
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

    /// Closes each ring that a completed replacement retired.
    pub fn retire_replaced(
        &mut self,
        graphics: &GraphicsContext,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<()> {
        if let Self::BorrowedTexture {
            session,
            replacements,
            ..
        } = self
        {
            while let Some(retired) = replacements.take_completed(session, wakes)? {
                close_ring(retired, Some(graphics.opengl()));
            }
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(
        &mut self,
        graphics: &GraphicsContext,
        _wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<bool> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            }
            | Self::BorrowedTexture {
                session,
                compositor,
                ..
            } => {
                // Without a new frame, the window keeps the one it already
                // shows.
                let Some(frame) = session.acquire_newest()? else {
                    return Ok(true);
                };
                require_cpu_complete_producer(frame)?;
                compositor.draw_frame(graphics.opengl(), frame)?;
            }
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
                ring,
                mut replacements,
            } => {
                session.close()?;
                for retired in replacements.take_all() {
                    close_ring(retired, gl);
                }
                compositor.close(gl);
                close_ring(ring, gl);
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

fn ring(gl: &OpenGLContext, viewport: Viewport) -> maplibre_native_ffi::Result<Box<OpenGLRing>> {
    let texture = || {
        OpenGLBorrowedTexture::new(gl, viewport)
            .map_err(|error| compositor_error(format!("OpenGL texture creation failed: {error}")))
    };
    let first = texture()?;
    match texture() {
        Ok(second) => Ok(Box::new([first, second])),
        Err(error) => {
            first.close(Some(gl));
            Err(error)
        }
    }
}

fn close_ring(ring: Box<OpenGLRing>, gl: Option<&OpenGLContext>) {
    for texture in *ring {
        texture.close(gl);
    }
}

fn borrowed_descriptor(
    context: OpenglContextDescriptor,
    ring: &OpenGLRing,
    viewport: Viewport,
) -> OpenglBorrowedTextureDescriptor {
    OpenglBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        context,
        textures: ring
            .iter()
            .map(|texture| OpenglBorrowedTexture::new(texture.texture()))
            .collect(),
        target: ring[0].target(),
    }
}
