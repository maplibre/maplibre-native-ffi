use maplibre_native_ffi::ScreenPoint;
use winit::event::{ElementState, MouseButton, MouseScrollDelta, WindowEvent};
use winit::keyboard::{KeyCode, ModifiersState, PhysicalKey};

use std::error::Error;

use crate::map_state::MapState;
use crate::viewport::Viewport;

const DRAG_ROTATE_FACTOR: f64 = 0.5;
const DRAG_PITCH_FACTOR: f64 = 0.5;
const KEYBOARD_PAN: f64 = 120.0;
const KEYBOARD_ZOOM: f64 = 1.25;
const KEYBOARD_BEARING: f64 = 10.0;
const KEYBOARD_PITCH: f64 = 5.0;
const KEYBOARD_ANIMATION_MS: f64 = 160.0;
const RESET_ANIMATION_MS: f64 = 220.0;

/// Decodes host input and updates the map in logical map coordinates.
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

    /// Reports whether the event changed the camera.
    pub fn handle(
        &mut self,
        event: &WindowEvent,
        viewport: Viewport,
        map: &mut MapState,
    ) -> Result<bool, Box<dyn Error>> {
        match event {
            WindowEvent::CursorMoved { position, .. } => self.cursor(
                position.x / viewport.scale_factor,
                position.y / viewport.scale_factor,
                map,
            ),
            WindowEvent::MouseInput { state, button, .. } => self.mouse(*button, *state, map),
            WindowEvent::MouseWheel { delta, .. } => self.wheel(viewport, *delta, map),
            WindowEvent::KeyboardInput { event, .. } => {
                keyboard(viewport, event.physical_key, event.state, map)
            }
            WindowEvent::ModifiersChanged(modifiers) => {
                self.modifiers = modifiers.state();
                Ok(false)
            }
            _ => Ok(false),
        }
    }

    fn cursor(&mut self, x: f64, y: f64, map: &mut MapState) -> Result<bool, Box<dyn Error>> {
        let dx = x - self.last_x;
        let dy = y - self.last_y;
        self.last_x = x;
        self.last_y = y;
        self.cursor_x = x;
        self.cursor_y = y;

        if self.right_down || (self.left_down && self.modifiers.control_key()) {
            if dx != 0.0 {
                map.adjust_bearing(dx * DRAG_ROTATE_FACTOR, None)?;
            }
            if dy != 0.0 {
                map.adjust_pitch(dy * DRAG_PITCH_FACTOR, None)?;
            }
            Ok(dx != 0.0 || dy != 0.0)
        } else if self.left_down && (dx != 0.0 || dy != 0.0) {
            map.move_by(dx, dy, None)?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn mouse(
        &mut self,
        button: MouseButton,
        state: ElementState,
        map: &mut MapState,
    ) -> Result<bool, Box<dyn Error>> {
        let was_dragging = self.dragging();
        match button {
            MouseButton::Left => self.left_down = state == ElementState::Pressed,
            MouseButton::Right => self.right_down = state == ElementState::Pressed,
            _ => return Ok(false),
        }
        if state == ElementState::Pressed {
            map.cancel_transitions()?;
        }
        if self.dragging() != was_dragging {
            map.set_gesture_in_progress(self.dragging())?;
        }
        Ok(false)
    }

    fn dragging(&self) -> bool {
        self.left_down || self.right_down
    }

    fn wheel(
        &mut self,
        viewport: Viewport,
        delta: MouseScrollDelta,
        map: &mut MapState,
    ) -> Result<bool, Box<dyn Error>> {
        let lines = match delta {
            MouseScrollDelta::LineDelta(_, y) => f64::from(y),
            MouseScrollDelta::PixelDelta(position) => position.y / viewport.scale_factor / 120.0,
        };
        if lines == 0.0 {
            return Ok(false);
        }
        map.scale_by(
            2.0_f64.powf(lines * 0.25),
            ScreenPoint::new(self.cursor_x, self.cursor_y),
            None,
        )?;
        Ok(true)
    }
}

fn keyboard(
    viewport: Viewport,
    physical_key: PhysicalKey,
    state: ElementState,
    map: &mut MapState,
) -> Result<bool, Box<dyn Error>> {
    if state != ElementState::Pressed {
        return Ok(false);
    }
    let PhysicalKey::Code(code) = physical_key else {
        return Ok(false);
    };
    let center = ScreenPoint::new(
        f64::from(viewport.logical_width) / 2.0,
        f64::from(viewport.logical_height) / 2.0,
    );
    match code {
        KeyCode::ArrowLeft | KeyCode::KeyA => {
            map.move_by(KEYBOARD_PAN, 0.0, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::ArrowRight | KeyCode::KeyD => {
            map.move_by(-KEYBOARD_PAN, 0.0, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::ArrowUp | KeyCode::KeyW => {
            map.move_by(0.0, KEYBOARD_PAN, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::ArrowDown | KeyCode::KeyS => {
            map.move_by(0.0, -KEYBOARD_PAN, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::Equal | KeyCode::NumpadAdd => {
            map.scale_by(KEYBOARD_ZOOM, center, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::Minus | KeyCode::NumpadSubtract => {
            map.scale_by(1.0 / KEYBOARD_ZOOM, center, Some(KEYBOARD_ANIMATION_MS))?
        }
        KeyCode::KeyQ => map.adjust_bearing(-KEYBOARD_BEARING, Some(KEYBOARD_ANIMATION_MS))?,
        KeyCode::KeyE => map.adjust_bearing(KEYBOARD_BEARING, Some(KEYBOARD_ANIMATION_MS))?,
        KeyCode::BracketRight => map.adjust_pitch(KEYBOARD_PITCH, Some(KEYBOARD_ANIMATION_MS))?,
        KeyCode::BracketLeft => map.adjust_pitch(-KEYBOARD_PITCH, Some(KEYBOARD_ANIMATION_MS))?,
        KeyCode::Digit0 | KeyCode::Numpad0 => map.reset_orientation(RESET_ANIMATION_MS)?,
        _ => return Ok(false),
    }
    Ok(true)
}
