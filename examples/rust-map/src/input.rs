use std::error::Error;

use maplibre_native::{AnimationOptions, CameraOptions, MapHandle, ScreenPoint};
use winit::event::{ElementState, MouseButton, MouseScrollDelta, WindowEvent};
use winit::keyboard::{KeyCode, ModifiersState, PhysicalKey};

use crate::viewport::Viewport;

const DRAG_ROTATE_FACTOR: f64 = 0.5;
const DRAG_PITCH_FACTOR: f64 = 0.5;
const KEYBOARD_PAN: f64 = 120.0;
const KEYBOARD_ZOOM: f64 = 1.25;
const KEYBOARD_BEARING: f64 = 10.0;
const KEYBOARD_PITCH: f64 = 5.0;

#[derive(Default)]
pub struct Controller {
    left_down: bool,
    right_down: bool,
    modifiers: ModifiersState,
    last_x: f64,
    last_y: f64,
    cursor_x: f64,
    cursor_y: f64,
}

impl Controller {
    pub fn print_controls() {
        println!("Controls:");
        println!("  left drag: pan");
        println!("  right drag or Ctrl+left drag: rotate with X, pitch with Y");
        println!("  scroll: zoom at cursor");
        println!("  arrows or WASD: pan");
        println!("  + / -: zoom at center");
        println!("  Q / E: rotate");
        println!("  ] / [: pitch");
        println!("  0: reset pitch and bearing");
    }

    pub fn handle(
        &mut self,
        event: &WindowEvent,
        map: &MapHandle,
        viewport: Viewport,
    ) -> Result<bool, Box<dyn Error>> {
        match event {
            WindowEvent::CursorMoved { position, .. } => self.cursor(
                map,
                position.x / viewport.scale_factor,
                position.y / viewport.scale_factor,
            ),
            WindowEvent::MouseInput { state, button, .. } => self.mouse(map, *button, *state),
            WindowEvent::MouseWheel { delta, .. } => self.wheel(map, viewport, *delta),
            WindowEvent::KeyboardInput { event, .. } => {
                self.keyboard(map, viewport, event.physical_key, event.state)
            }
            WindowEvent::ModifiersChanged(modifiers) => {
                self.modifiers = modifiers.state();
                Ok(false)
            }
            _ => Ok(false),
        }
    }

    fn cursor(&mut self, map: &MapHandle, x: f64, y: f64) -> Result<bool, Box<dyn Error>> {
        let dx = x - self.last_x;
        let dy = y - self.last_y;
        self.last_x = x;
        self.last_y = y;
        self.cursor_x = x;
        self.cursor_y = y;

        if self.right_down || (self.left_down && self.modifiers.control_key()) {
            if dx != 0.0 {
                let bearing = map.camera()?.bearing.unwrap_or(0.0) + dx * DRAG_ROTATE_FACTOR;
                let mut camera = CameraOptions::default();
                camera.bearing = Some(bearing);
                map.jump_to(&camera)?;
            }
            if dy != 0.0 {
                map.pitch_by(dy * DRAG_PITCH_FACTOR)?;
            }
            Ok(dx != 0.0 || dy != 0.0)
        } else if self.left_down && (dx != 0.0 || dy != 0.0) {
            map.move_by(dx, dy)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn mouse(
        &mut self,
        map: &MapHandle,
        button: MouseButton,
        state: ElementState,
    ) -> Result<bool, Box<dyn Error>> {
        match button {
            MouseButton::Left => self.left_down = state == ElementState::Pressed,
            MouseButton::Right => self.right_down = state == ElementState::Pressed,
            _ => return Ok(false),
        }
        if state == ElementState::Pressed {
            map.cancel_transitions()?;
        }
        Ok(false)
    }

    fn wheel(
        &mut self,
        map: &MapHandle,
        viewport: Viewport,
        delta: MouseScrollDelta,
    ) -> Result<bool, Box<dyn Error>> {
        let lines = match delta {
            MouseScrollDelta::LineDelta(_, y) => f64::from(y),
            MouseScrollDelta::PixelDelta(position) => position.y / viewport.scale_factor / 120.0,
        };
        if lines == 0.0 {
            return Ok(false);
        }
        let scale = 2.0_f64.powf(lines * 0.25);
        map.scale_by(scale, Some(ScreenPoint::new(self.cursor_x, self.cursor_y)))?;
        Ok(true)
    }

    fn keyboard(
        &mut self,
        map: &MapHandle,
        viewport: Viewport,
        physical_key: PhysicalKey,
        state: ElementState,
    ) -> Result<bool, Box<dyn Error>> {
        if state != ElementState::Pressed {
            return Ok(false);
        }
        let PhysicalKey::Code(code) = physical_key else {
            return Ok(false);
        };
        let mut animation = AnimationOptions::default();
        animation.duration_ms = Some(160.0);
        let mut reset_animation = AnimationOptions::default();
        reset_animation.duration_ms = Some(220.0);
        let center = Some(ScreenPoint::new(
            f64::from(viewport.logical_width) / 2.0,
            f64::from(viewport.logical_height) / 2.0,
        ));
        match code {
            KeyCode::ArrowLeft | KeyCode::KeyA => {
                map.move_by_animated(KEYBOARD_PAN, 0.0, Some(&animation))?
            }
            KeyCode::ArrowRight | KeyCode::KeyD => {
                map.move_by_animated(-KEYBOARD_PAN, 0.0, Some(&animation))?
            }
            KeyCode::ArrowUp | KeyCode::KeyW => {
                map.move_by_animated(0.0, KEYBOARD_PAN, Some(&animation))?
            }
            KeyCode::ArrowDown | KeyCode::KeyS => {
                map.move_by_animated(0.0, -KEYBOARD_PAN, Some(&animation))?
            }
            KeyCode::Equal | KeyCode::NumpadAdd => {
                map.scale_by_animated(KEYBOARD_ZOOM, center, Some(&animation))?
            }
            KeyCode::Minus | KeyCode::NumpadSubtract => {
                map.scale_by_animated(1.0 / KEYBOARD_ZOOM, center, Some(&animation))?
            }
            KeyCode::KeyQ => adjust_bearing(map, -KEYBOARD_BEARING, &animation)?,
            KeyCode::KeyE => adjust_bearing(map, KEYBOARD_BEARING, &animation)?,
            KeyCode::BracketRight => adjust_pitch(map, KEYBOARD_PITCH, &animation)?,
            KeyCode::BracketLeft => adjust_pitch(map, -KEYBOARD_PITCH, &animation)?,
            KeyCode::Digit0 | KeyCode::Numpad0 => {
                let mut camera = CameraOptions::default();
                camera.bearing = Some(0.0);
                camera.pitch = Some(0.0);
                map.ease_to(&camera, Some(&reset_animation))?
            }
            _ => return Ok(false),
        }
        Ok(true)
    }
}

fn adjust_bearing(
    map: &MapHandle,
    delta: f64,
    animation: &AnimationOptions,
) -> maplibre_native::Result<()> {
    let bearing = map.camera()?.bearing.unwrap_or(0.0) + delta;
    let mut camera = CameraOptions::default();
    camera.bearing = Some(bearing);
    map.ease_to(&camera, Some(animation))
}

fn adjust_pitch(
    map: &MapHandle,
    delta: f64,
    animation: &AnimationOptions,
) -> maplibre_native::Result<()> {
    let pitch = (map.camera()?.pitch.unwrap_or(0.0) + delta).clamp(0.0, 60.0);
    let mut camera = CameraOptions::default();
    camera.pitch = Some(pitch);
    map.ease_to(&camera, Some(animation))
}
