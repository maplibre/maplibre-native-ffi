use maplibre_native_ffi::ScreenPoint;
use winit::event::{ElementState, MouseButton, MouseScrollDelta, WindowEvent};
use winit::keyboard::{KeyCode, ModifiersState, PhysicalKey};

use crate::map_state::CameraCommand;
use crate::viewport::Viewport;

const DRAG_ROTATE_FACTOR: f64 = 0.5;
const DRAG_PITCH_FACTOR: f64 = 0.5;
const KEYBOARD_PAN: f64 = 120.0;
const KEYBOARD_ZOOM: f64 = 1.25;
const KEYBOARD_BEARING: f64 = 10.0;
const KEYBOARD_PITCH: f64 = 5.0;
const KEYBOARD_ANIMATION_MS: f64 = 160.0;
const RESET_ANIMATION_MS: f64 = 220.0;

/// Decodes host input into camera commands in logical map coordinates.
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

    /// Reports whether the event submitted a camera change.
    pub fn handle<F, E>(
        &mut self,
        event: &WindowEvent,
        viewport: Viewport,
        mut submit: F,
    ) -> Result<bool, E>
    where
        F: FnMut(CameraCommand) -> Result<(), E>,
    {
        match event {
            WindowEvent::CursorMoved { position, .. } => self.cursor(
                position.x / viewport.scale_factor,
                position.y / viewport.scale_factor,
                &mut submit,
            ),
            WindowEvent::MouseInput { state, button, .. } => {
                self.mouse(*button, *state, &mut submit)
            }
            WindowEvent::MouseWheel { delta, .. } => self.wheel(viewport, *delta, &mut submit),
            WindowEvent::KeyboardInput { event, .. } => {
                keyboard(viewport, event.physical_key, event.state, &mut submit)
            }
            WindowEvent::ModifiersChanged(modifiers) => {
                self.modifiers = modifiers.state();
                Ok(false)
            }
            _ => Ok(false),
        }
    }

    fn cursor<F, E>(&mut self, x: f64, y: f64, submit: &mut F) -> Result<bool, E>
    where
        F: FnMut(CameraCommand) -> Result<(), E>,
    {
        let dx = x - self.last_x;
        let dy = y - self.last_y;
        self.last_x = x;
        self.last_y = y;
        self.cursor_x = x;
        self.cursor_y = y;

        if self.right_down || (self.left_down && self.modifiers.control_key()) {
            if dx != 0.0 {
                submit(CameraCommand::AdjustBearing {
                    delta: dx * DRAG_ROTATE_FACTOR,
                })?;
            }
            if dy != 0.0 {
                submit(CameraCommand::PitchBy {
                    delta: dy * DRAG_PITCH_FACTOR,
                })?;
            }
            Ok(dx != 0.0 || dy != 0.0)
        } else if self.left_down && (dx != 0.0 || dy != 0.0) {
            submit(CameraCommand::MoveBy { dx, dy })?;
            Ok(true)
        } else {
            Ok(false)
        }
    }

    fn mouse<F, E>(
        &mut self,
        button: MouseButton,
        state: ElementState,
        submit: &mut F,
    ) -> Result<bool, E>
    where
        F: FnMut(CameraCommand) -> Result<(), E>,
    {
        let was_dragging = self.dragging();
        match button {
            MouseButton::Left => self.left_down = state == ElementState::Pressed,
            MouseButton::Right => self.right_down = state == ElementState::Pressed,
            _ => return Ok(false),
        }
        if state == ElementState::Pressed {
            submit(CameraCommand::CancelTransitions)?;
        }
        if self.dragging() != was_dragging {
            submit(CameraCommand::SetGestureInProgress {
                in_progress: self.dragging(),
            })?;
        }
        Ok(false)
    }

    fn dragging(&self) -> bool {
        self.left_down || self.right_down
    }

    fn wheel<F, E>(
        &mut self,
        viewport: Viewport,
        delta: MouseScrollDelta,
        submit: &mut F,
    ) -> Result<bool, E>
    where
        F: FnMut(CameraCommand) -> Result<(), E>,
    {
        let lines = match delta {
            MouseScrollDelta::LineDelta(_, y) => f64::from(y),
            MouseScrollDelta::PixelDelta(position) => position.y / viewport.scale_factor / 120.0,
        };
        if lines == 0.0 {
            return Ok(false);
        }
        submit(CameraCommand::ScaleBy {
            scale: 2.0_f64.powf(lines * 0.25),
            anchor: ScreenPoint::new(self.cursor_x, self.cursor_y),
        })?;
        Ok(true)
    }
}

fn keyboard<F, E>(
    viewport: Viewport,
    physical_key: PhysicalKey,
    state: ElementState,
    submit: &mut F,
) -> Result<bool, E>
where
    F: FnMut(CameraCommand) -> Result<(), E>,
{
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
    let command = match code {
        KeyCode::ArrowLeft | KeyCode::KeyA => pan(KEYBOARD_PAN, 0.0),
        KeyCode::ArrowRight | KeyCode::KeyD => pan(-KEYBOARD_PAN, 0.0),
        KeyCode::ArrowUp | KeyCode::KeyW => pan(0.0, KEYBOARD_PAN),
        KeyCode::ArrowDown | KeyCode::KeyS => pan(0.0, -KEYBOARD_PAN),
        KeyCode::Equal | KeyCode::NumpadAdd => zoom(KEYBOARD_ZOOM, center),
        KeyCode::Minus | KeyCode::NumpadSubtract => zoom(1.0 / KEYBOARD_ZOOM, center),
        KeyCode::KeyQ => CameraCommand::AdjustBearingAnimated {
            delta: -KEYBOARD_BEARING,
            duration_ms: KEYBOARD_ANIMATION_MS,
        },
        KeyCode::KeyE => CameraCommand::AdjustBearingAnimated {
            delta: KEYBOARD_BEARING,
            duration_ms: KEYBOARD_ANIMATION_MS,
        },
        KeyCode::BracketRight => CameraCommand::AdjustPitchAnimated {
            delta: KEYBOARD_PITCH,
            duration_ms: KEYBOARD_ANIMATION_MS,
        },
        KeyCode::BracketLeft => CameraCommand::AdjustPitchAnimated {
            delta: -KEYBOARD_PITCH,
            duration_ms: KEYBOARD_ANIMATION_MS,
        },
        KeyCode::Digit0 | KeyCode::Numpad0 => CameraCommand::ResetOrientation {
            duration_ms: RESET_ANIMATION_MS,
        },
        _ => return Ok(false),
    };
    submit(command)?;
    Ok(true)
}

fn pan(dx: f64, dy: f64) -> CameraCommand {
    CameraCommand::MoveByAnimated {
        dx,
        dy,
        duration_ms: KEYBOARD_ANIMATION_MS,
    }
}

fn zoom(scale: f64, anchor: ScreenPoint) -> CameraCommand {
    CameraCommand::ScaleByAnimated {
        scale,
        anchor,
        duration_ms: KEYBOARD_ANIMATION_MS,
    }
}
