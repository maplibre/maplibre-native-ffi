use std::error::Error as StdError;

use maplibre_native::{Error, ErrorKind, MapHandle, RenderSessionHandle};

use crate::graphics::GraphicsContext;
use crate::metal::{MetalBorrowedTexture, MetalContext, MetalTextureCompositor};
use crate::render_target::{Mode, extent};
use crate::viewport::Viewport;

pub enum RenderTarget {
    OwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
    },
    BorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
        texture: Box<MetalBorrowedTexture>,
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
    ) -> maplibre_native::Result<Self> {
        let metal = graphics.metal();
        match mode {
            Mode::OwnedTexture => attach_owned_texture(map, metal, viewport),
            Mode::BorrowedTexture => attach_borrowed_texture(map, metal, viewport),
            Mode::NativeSurface => attach_surface(map, metal, viewport),
        }
    }

    pub fn needs_reattach_on_resize(&self) -> bool {
        matches!(self, Self::BorrowedTexture { .. })
    }

    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native::Result<()> {
        match self {
            Self::BorrowedTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            Self::OwnedTexture { session, .. } | Self::Surface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(&mut self, _graphics: &GraphicsContext) -> maplibre_native::Result<()> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                session.render_update()?;
                let frame = session.acquire_metal_owned_texture_frame()?;
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
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                session.render_update()?;
                compositor.draw_texture(texture.texture())
            }
            Self::Surface { session } => session.render_update(),
        }
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                drop(compositor);
                session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            Self::BorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                drop(compositor);
                let result = session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>);
                drop(texture);
                result
            }
            Self::Surface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
        }
    }
}

fn attach_owned_texture(
    map: &MapHandle,
    metal: &MetalContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let descriptor = maplibre_native::MetalOwnedTextureDescriptor::new(
        extent(viewport),
        metal.context_descriptor(),
    );
    let session = map.attach_metal_owned_texture(&descriptor)?;
    let compositor = match MetalTextureCompositor::new(metal) {
        Ok(compositor) => compositor,
        Err(error) => {
            let mut message = format!("Metal texture compositor creation failed: {error:?}");
            if let Err(close_error) = session.close() {
                message.push_str(&format!("; render session cleanup failed: {close_error}"));
            }
            return Err(compositor_error(message));
        }
    };
    Ok(RenderTarget::OwnedTexture {
        session,
        compositor: Box::new(compositor),
    })
}

fn attach_borrowed_texture(
    map: &MapHandle,
    metal: &MetalContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let texture = MetalBorrowedTexture::new(metal, viewport)?;
    let descriptor =
        maplibre_native::MetalBorrowedTextureDescriptor::new(extent(viewport), texture.pointer());
    let session = map.attach_metal_borrowed_texture(&descriptor)?;
    let compositor = match MetalTextureCompositor::new(metal) {
        Ok(compositor) => compositor,
        Err(error) => {
            let mut message = format!("Metal texture compositor creation failed: {error:?}");
            if let Err(close_error) = session.close() {
                message.push_str(&format!("; render session cleanup failed: {close_error}"));
            }
            return Err(compositor_error(message));
        }
    };
    Ok(RenderTarget::BorrowedTexture {
        session,
        compositor: Box::new(compositor),
        texture: Box::new(texture),
    })
}

fn attach_surface(
    map: &MapHandle,
    metal: &MetalContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let descriptor = maplibre_native::MetalSurfaceDescriptor::new(
        extent(viewport),
        metal.context_descriptor(),
        metal.layer_pointer(),
    );
    Ok(RenderTarget::Surface {
        session: map.attach_metal_surface(&descriptor)?,
    })
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
