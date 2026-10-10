use std::error::Error as StdError;

use maplibre_native_ffi::{MapHandle, RenderDriverKind};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::metal::{MetalBorrowedTexture, MetalTextureCompositor};
use crate::render_target::{
    Mode, RING_DEPTH, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;

/// The textures of a borrowed ring, one per slot.
type MetalRing = [MetalBorrowedTexture; RING_DEPTH];

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<MetalTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<MetalTextureCompositor>,
        /// The ring the session renders into.
        ring: Box<MetalRing>,
        replacements: Replacements<Box<MetalRing>>,
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
        let metal = graphics.metal();
        // Every Metal target accepts a core worker, which renders on its own
        // thread.
        let options = attach_options(wakes, mode, RenderDriverKind::CoreWorker);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = maplibre_native_ffi::MetalOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: metal.context_descriptor(),
                };
                let session = Session::new(
                    unsafe { map.metal_owned_texture_attach(&descriptor, &options) }?,
                    &options,
                    mode,
                    wakes,
                )?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor(metal, viewport)?),
                })
            }
            Mode::BorrowedTexture => {
                let ring = ring(metal, viewport)?;
                let session = Session::new(
                    unsafe {
                        map.metal_borrowed_texture_attach(
                            &borrowed_descriptor(&ring, viewport),
                            &options,
                        )
                    }?,
                    &options,
                    mode,
                    wakes,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(metal, viewport)?),
                    ring,
                    replacements: Replacements::default(),
                })
            }
            Mode::NativeSurface => {
                let descriptor = maplibre_native_ffi::MetalSurfaceDescriptor::new(
                    extent(viewport),
                    metal.context_descriptor(),
                    metal.layer_pointer(),
                );
                Ok(Self::Surface {
                    session: Session::new(
                        unsafe { map.metal_surface_attach(&descriptor, &options) }?,
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
        match self {
            Self::BorrowedTexture {
                session,
                ring: current,
                replacements,
                ..
            } => {
                graphics.metal().resize(viewport);
                // A replacement is refused while the host holds a frame, and
                // the window keeps showing what it last presented.
                session.release_held()?;
                let replacement = ring(graphics.metal(), viewport)?;
                let completion = unsafe {
                    session
                        .handle()
                        .metal_borrowed_texture_set_target(&borrowed_descriptor(
                            &replacement,
                            viewport,
                        ))
                }?;
                let retired = std::mem::replace(current, replacement);
                replacements.push(completion, retired, wakes);
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::OwnedTexture { session, .. } => {
                graphics.metal().resize(viewport);
                session.resize(viewport)?;
                Ok(())
            }
            // The session sets the layer's drawable size.
            Self::Surface { session } => {
                session.resize(viewport)?;
                Ok(())
            }
        }
    }

    /// Releases each ring that a completed replacement retired.
    pub fn retire_replaced(
        &mut self,
        _graphics: &GraphicsContext,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<()> {
        if let Self::BorrowedTexture {
            session,
            replacements,
            ..
        } = self
        {
            while replacements.take_completed(session, wakes)?.is_some() {}
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(
        &mut self,
        _graphics: &GraphicsContext,
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
                compositor.draw(frame)
            }
            // The driver already presented the frame.
            Self::Surface { .. } => Ok(true),
        }
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session.close(),
        }
    }
}

/// Creates the compositor for a texture mode, which sizes the layer's drawable
/// to the viewport. A surface session sizes it itself.
fn compositor(
    metal: &crate::metal::MetalContext,
    viewport: Viewport,
) -> maplibre_native_ffi::Result<MetalTextureCompositor> {
    metal.resize(viewport);
    MetalTextureCompositor::new(metal)
        .map_err(|error| compositor_error(format!("Metal compositor creation failed: {error:?}")))
}

fn ring(
    metal: &crate::metal::MetalContext,
    viewport: Viewport,
) -> maplibre_native_ffi::Result<Box<MetalRing>> {
    Ok(Box::new([
        MetalBorrowedTexture::new(metal, viewport)?,
        MetalBorrowedTexture::new(metal, viewport)?,
    ]))
}

fn borrowed_descriptor(
    ring: &MetalRing,
    viewport: Viewport,
) -> maplibre_native_ffi::MetalBorrowedTextureDescriptor {
    maplibre_native_ffi::MetalBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        textures: ring
            .iter()
            .map(|texture| maplibre_native_ffi::MetalBorrowedTexture::new(texture.pointer()))
            .collect(),
    }
}
