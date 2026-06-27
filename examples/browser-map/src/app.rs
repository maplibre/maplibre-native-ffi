use maplibre_native::{
    AnimationOptions, CameraOptions, Error, ErrorKind, LatLng, NativePointer, RenderBackendMask,
    ScreenPoint, WebGPUContextDescriptor,
};

use crate::diagnostics::{log_startup, log_viewport};
use crate::map_state::MapState;
use crate::viewport::Viewport;

const KEYBOARD_ANIMATION_MS: f64 = 160.0;

pub struct App {
    viewport: Viewport,
    map_state: MapState,
    render_pending: bool,
}

impl App {
    pub fn new(
        viewport: Viewport,
        webgpu_device: NativePointer,
        webgpu_queue: NativePointer,
    ) -> Result<Self, Error> {
        let supported = maplibre_native::supported_render_backends();
        if !supported.contains(RenderBackendMask::WEBGPU) {
            return Err(Error::new(
                ErrorKind::Unsupported,
                None,
                "WebGPU backend is not available in this build",
            ));
        }

        log_startup(supported);
        log_viewport(&viewport);

        let webgpu_context = WebGPUContextDescriptor::new(webgpu_device, webgpu_queue);
        let map_state = MapState::new(&viewport, &webgpu_context)?;

        Ok(Self {
            viewport,
            map_state,
            render_pending: true,
        })
    }

    pub fn render_frame(&mut self) -> Result<bool, Error> {
        if self.map_state.pump_runtime()? {
            self.request_render();
        }
        if !self.render_pending {
            return Ok(false);
        }
        match self.map_state.render_update() {
            Ok(()) => {
                self.render_pending = false;
                Ok(true)
            }
            Err(error) if error.kind() == ErrorKind::InvalidState => Ok(false),
            Err(error) => Err(error),
        }
    }

    pub fn resize(&mut self, viewport: Viewport) -> Result<(), Error> {
        self.viewport = viewport;
        self.map_state.resize(&self.viewport)?;
        self.request_render();
        log_viewport(&self.viewport);
        Ok(())
    }

    pub fn acquire_owned_texture(&mut self) -> Result<usize, Error> {
        self.map_state.acquire_owned_texture()
    }

    pub fn release_owned_texture_frame(&mut self) -> Result<(), Error> {
        self.map_state.release_owned_texture_frame()
    }

    pub fn move_by(&mut self, delta_x: f64, delta_y: f64) -> Result<(), Error> {
        self.map_state.map()?.move_by(delta_x, delta_y)?;
        self.request_render();
        Ok(())
    }

    pub fn scale_by(&mut self, scale: f64, x: f64, y: f64) -> Result<(), Error> {
        self.map_state
            .map()?
            .scale_by(scale, Some(ScreenPoint { x, y }))?;
        self.request_render();
        Ok(())
    }

    pub fn rotate_pitch_by(&mut self, bearing_delta: f64, pitch_delta: f64) -> Result<(), Error> {
        let camera = self.map_state.map()?.camera()?;
        self.set_orientation(
            camera.bearing.unwrap_or(0.0) + bearing_delta,
            camera.pitch.unwrap_or(0.0) + pitch_delta,
            false,
        )
    }

    pub fn rotate_by(&mut self, bearing_delta: f64) -> Result<(), Error> {
        let camera = self.map_state.map()?.camera()?;
        self.set_orientation(
            camera.bearing.unwrap_or(0.0) + bearing_delta,
            camera.pitch.unwrap_or(0.0),
            true,
        )
    }

    pub fn pitch_by(&mut self, pitch_delta: f64) -> Result<(), Error> {
        let camera = self.map_state.map()?.camera()?;
        self.set_orientation(
            camera.bearing.unwrap_or(0.0),
            camera.pitch.unwrap_or(0.0) + pitch_delta,
            true,
        )
    }

    pub fn reset_orientation(&mut self) -> Result<(), Error> {
        self.set_orientation(0.0, 0.0, true)
    }

    pub fn jump_to(
        &mut self,
        longitude: f64,
        latitude: f64,
        zoom: f64,
        bearing: f64,
        pitch: f64,
    ) -> Result<(), Error> {
        let mut camera = CameraOptions::default();
        camera.center = Some(LatLng::new(latitude, longitude));
        camera.zoom = Some(zoom);
        camera.bearing = Some(bearing);
        camera.pitch = Some(pitch.clamp(0.0, 60.0));
        self.map_state.map()?.jump_to(&camera)?;
        self.request_render();
        Ok(())
    }

    pub fn cancel_transitions(&mut self) -> Result<(), Error> {
        self.map_state.map()?.cancel_transitions()?;
        self.request_render();
        Ok(())
    }

    fn set_orientation(&mut self, bearing: f64, pitch: f64, animated: bool) -> Result<(), Error> {
        let mut camera = CameraOptions::default();
        camera.bearing = Some(bearing);
        camera.pitch = Some(pitch.clamp(0.0, 60.0));
        if animated {
            self.map_state
                .map()?
                .ease_to(&camera, Some(&animation(KEYBOARD_ANIMATION_MS)))?;
        } else {
            self.map_state.map()?.jump_to(&camera)?;
        }
        self.request_render();
        Ok(())
    }

    fn request_render(&mut self) {
        self.render_pending = true;
    }
}

fn animation(duration_ms: f64) -> AnimationOptions {
    let mut animation = AnimationOptions::default();
    animation.duration_ms = Some(duration_ms);
    animation
}
