use std::error::Error as StdError;

use maplibre_native_ffi::{GpuSync, MapHandle};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::metal::{MetalBorrowedTexture, MetalTextureCompositor};
use crate::render_target::{
    Mode, Replacements, Session, attach_options, compositor_error, extent,
    require_cpu_complete_producer,
};
use crate::shell::Wakes;
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        session: Session,
        compositor: Box<MetalTextureCompositor>,
    },
    BorrowedTexture {
        session: Session,
        compositor: Box<MetalTextureCompositor>,
        /// The texture the session renders into as far as completed
        /// replacements show.
        texture: Box<MetalBorrowedTexture>,
        replacements: Replacements<MetalBorrowedTexture>,
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
        let options = attach_options(wakes, mode);
        match mode {
            Mode::OwnedTexture => {
                let descriptor = maplibre_native_ffi::MetalOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: metal.context_descriptor(),
                };
                let session = Session::new(
                    unsafe { map.metal_owned_texture_attach(&descriptor, &options) }?,
                    false,
                )?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor(metal)?),
                })
            }
            Mode::BorrowedTexture => {
                let texture = MetalBorrowedTexture::new(metal, viewport)?;
                let session = Session::new(
                    unsafe {
                        map.metal_borrowed_texture_attach(
                            &borrowed_descriptor(&texture, viewport),
                            &options,
                        )
                    }?,
                    false,
                )?;
                Ok(Self::BorrowedTexture {
                    session,
                    compositor: Box::new(compositor(metal)?),
                    texture: Box::new(texture),
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
            Self::BorrowedTexture {
                session,
                replacements,
                ..
            } => {
                let replacement = MetalBorrowedTexture::new(graphics.metal(), viewport)?;
                let completion = unsafe {
                    session
                        .handle()
                        .metal_borrowed_texture_set_target(&borrowed_descriptor(
                            &replacement,
                            viewport,
                        ))
                }?;
                replacements.push(completion, replacement);
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::OwnedTexture { session, .. } | Self::Surface { session } => {
                session.resize(viewport)?;
                Ok(())
            }
        }
    }

    /// Services caller-driver work, then releases what completed target
    /// replacements retired.
    pub fn service(&mut self, _graphics: &GraphicsContext) -> maplibre_native_ffi::Result<()> {
        self.session_mut().service()?;
        if let Self::BorrowedTexture {
            texture,
            replacements,
            ..
        } = self
        {
            while let Some(replacement) = replacements.take_completed()? {
                **texture = replacement;
            }
        }
        Ok(())
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(&mut self, _graphics: &GraphicsContext) -> maplibre_native_ffi::Result<bool> {
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
                let presented = compositor.draw(&frame);
                frame.release(&GpuSync::default())?;
                presented
            }
            Self::BorrowedTexture {
                compositor,
                texture,
                ..
            } => compositor.draw_texture(texture.texture()),
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

fn compositor(
    metal: &crate::metal::MetalContext,
) -> maplibre_native_ffi::Result<MetalTextureCompositor> {
    MetalTextureCompositor::new(metal)
        .map_err(|error| compositor_error(format!("Metal compositor creation failed: {error:?}")))
}

fn borrowed_descriptor(
    texture: &MetalBorrowedTexture,
    viewport: Viewport,
) -> maplibre_native_ffi::MetalBorrowedTextureDescriptor {
    maplibre_native_ffi::MetalBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        texture: texture.pointer(),
    }
}
