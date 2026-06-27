use maplibre_native::{
    Error, ErrorKind, MapHandle, MapMode, MapOptions, RenderSessionHandle, RuntimeEventPayload,
    RuntimeEventSource, RuntimeEventType, RuntimeHandle, RuntimeOptions, WebGPUContextDescriptor,
    WebGPUOwnedTextureFrameHandle,
};

use crate::render_target;
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

pub struct MapState {
    runtime: Option<RuntimeHandle>,
    map: Option<MapHandle>,
    session: Option<RenderSessionHandle>,
    owned_frame: Option<WebGPUOwnedTextureFrameHandle>,
}

impl MapState {
    pub fn new(viewport: &Viewport, context: &WebGPUContextDescriptor) -> Result<Self, Error> {
        let mut runtime_options = RuntimeOptions::default();
        runtime_options.cache_path = Some(":memory:".to_owned());
        let runtime = RuntimeHandle::with_options(&runtime_options)?;

        let mut map_options = MapOptions::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        );
        map_options.mode = MapMode::Continuous;
        let map = MapHandle::with_options(&runtime, &map_options)?;

        map.set_style_url(STYLE_URL)?;
        let session = render_target::attach(&map, viewport, context)?;

        Ok(Self {
            runtime: Some(runtime),
            map: Some(map),
            session: Some(session),
            owned_frame: None,
        })
    }

    pub fn pump_runtime(&mut self) -> Result<bool, Error> {
        let Some(runtime) = &self.runtime else {
            return Ok(false);
        };
        runtime.run_once()?;
        self.drain_events()
    }

    pub fn render_update(&self) -> Result<(), Error> {
        let Some(session) = &self.session else {
            return Ok(());
        };
        session.render_update()
    }

    pub fn resize(&mut self, viewport: &Viewport) -> Result<(), Error> {
        self.release_owned_texture_frame()?;
        if let Some(session) = &self.session {
            session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            )?;
        }
        Ok(())
    }

    pub fn acquire_owned_texture(&mut self) -> Result<usize, Error> {
        self.release_owned_texture_frame()?;
        let Some(session) = &self.session else {
            return Ok(0);
        };
        let frame = session.acquire_webgpu_owned_texture_frame()?;
        // SAFETY: The returned address is used immediately by JS and remains
        // valid until mln_browser_map_release_owned_texture_frame is called.
        let texture = unsafe { frame.texture()?.address() };
        self.owned_frame = Some(frame);
        Ok(texture)
    }

    pub fn release_owned_texture_frame(&mut self) -> Result<(), Error> {
        let Some(frame) = self.owned_frame.take() else {
            return Ok(());
        };
        frame.close().map_err(|error| error.into_error())
    }

    pub fn map(&self) -> Result<&MapHandle, Error> {
        self.map.as_ref().ok_or_else(closed_app_error)
    }

    fn drain_events(&mut self) -> Result<bool, Error> {
        let map_id = self.map.as_ref().map(MapHandle::id);
        let mut render_requested = false;
        loop {
            let event = match self.runtime.as_ref().map(RuntimeHandle::poll_event) {
                None => return Ok(render_requested),
                Some(Ok(Some(event))) => event,
                Some(Ok(None)) => return Ok(render_requested),
                Some(Err(error)) => return Err(error),
            };
            if Some(event.source) != map_id.map(RuntimeEventSource::Map) {
                continue;
            }
            match event.event_type {
                RuntimeEventType::MapRenderUpdateAvailable => render_requested = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload {
                        render_requested |= frame.needs_repaint;
                    }
                }
                RuntimeEventType::MapRenderError => {
                    eprintln!(
                        "render error: {}",
                        event.message.as_deref().unwrap_or("unknown render error")
                    );
                }
                _ => {}
            }
        }
    }
}

fn closed_app_error() -> Error {
    Error::new(
        ErrorKind::InvalidState,
        None,
        "browser map app is not initialized",
    )
}
