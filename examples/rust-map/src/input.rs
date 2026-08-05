use std::sync::mpsc::Sender;

use maplibre_native_ffi::ScreenPoint;
use winit::event::{ElementState, MouseButton, MouseScrollDelta, WindowEvent};
use winit::keyboard::{KeyCode, ModifiersState, PhysicalKey};

use crate::channel::CameraCommand;
use crate::viewport::Viewport;

const DRAG_ROTATE_FACTOR: f64 = 0.5;
const DRAG_PITCH_FACTOR: f64 = 0.5;
const KEYBOARD_PAN: f64 = 120.0;
const KEYBOARD_ZOOM: f64 = 1.25;
const KEYBOARD_BEARING: f64 = 10.0;
const KEYBOARD_PITCH: f64 = 5.0;
const KEYBOARD_ANIMATION_MS: f64 = 160.0;
const RESET_ANIMATION_MS: f64 = 220.0;

/// Decodes host input into camera commands on the render loop, converting to
/// logical map coordinates. The runtime loop applies the commands.
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

    /// Reports whether the camera changed.
    pub fn handle(
        &mut self,
        event: &WindowEvent,
        commands: &Sender<CameraCommand>,
        viewport: Viewport,
    ) -> bool {
        match event {
            WindowEvent::CursorMoved { position, .. } => self.cursor(
                commands,
                position.x / viewport.scale_factor,
                position.y / viewport.scale_factor,
            ),
            WindowEvent::MouseInput { state, button, .. } => self.mouse(commands, *button, *state),
            WindowEvent::MouseWheel { delta, .. } => self.wheel(commands, viewport, *delta),
            WindowEvent::KeyboardInput { event, .. } => {
                keyboard(commands, viewport, event.physical_key, event.state)
            }
            WindowEvent::ModifiersChanged(modifiers) => {
                self.modifiers = modifiers.state();
                false
            }
            _ => false,
        }
    }

    fn cursor(&mut self, commands: &Sender<CameraCommand>, x: f64, y: f64) -> bool {
        let dx = x - self.last_x;
        let dy = y - self.last_y;
        self.last_x = x;
        self.last_y = y;
        self.cursor_x = x;
        self.cursor_y = y;

        if self.right_down || (self.left_down && self.modifiers.control_key()) {
            if dx != 0.0 {
                push(
                    commands,
                    CameraCommand::AdjustBearing {
                        delta: dx * DRAG_ROTATE_FACTOR,
                    },
                );
            }
            if dy != 0.0 {
                push(
                    commands,
                    CameraCommand::PitchBy {
                        delta: dy * DRAG_PITCH_FACTOR,
                    },
                );
            }
            dx != 0.0 || dy != 0.0
        } else if self.left_down && (dx != 0.0 || dy != 0.0) {
            push(commands, CameraCommand::MoveBy { dx, dy });
            true
        } else {
            false
        }
    }

    fn mouse(
        &mut self,
        commands: &Sender<CameraCommand>,
        button: MouseButton,
        state: ElementState,
    ) -> bool {
        let was_dragging = self.dragging();
        match button {
            MouseButton::Left => self.left_down = state == ElementState::Pressed,
            MouseButton::Right => self.right_down = state == ElementState::Pressed,
            _ => return false,
        }
        if state == ElementState::Pressed {
            // Cancel first, so the running transition stops before the first
            // delta.
            push(commands, CameraCommand::CancelTransitions);
        }
        if self.dragging() != was_dragging {
            push(
                commands,
                CameraCommand::SetGestureInProgress {
                    in_progress: self.dragging(),
                },
            );
        }
        false
    }

    fn dragging(&self) -> bool {
        self.left_down || self.right_down
    }

    fn wheel(
        &mut self,
        commands: &Sender<CameraCommand>,
        viewport: Viewport,
        delta: MouseScrollDelta,
    ) -> bool {
        let lines = match delta {
            MouseScrollDelta::LineDelta(_, y) => f64::from(y),
            MouseScrollDelta::PixelDelta(position) => position.y / viewport.scale_factor / 120.0,
        };
        if lines == 0.0 {
            return false;
        }
        push(
            commands,
            CameraCommand::ScaleBy {
                scale: 2.0_f64.powf(lines * 0.25),
                anchor: ScreenPoint::new(self.cursor_x, self.cursor_y),
            },
        );
        true
    }
}

fn keyboard(
    commands: &Sender<CameraCommand>,
    viewport: Viewport,
    physical_key: PhysicalKey,
    state: ElementState,
) -> bool {
    if state != ElementState::Pressed {
        return false;
    }
    let PhysicalKey::Code(code) = physical_key else {
        return false;
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
        _ => return false,
    };
    push(commands, command);
    true
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

/// Drops the command if the queue is closed; the runtime loop has stopped and
/// the render loop reports that through the shared failure.
fn push(commands: &Sender<CameraCommand>, command: CameraCommand) {
    let _ = commands.send(command);
}
