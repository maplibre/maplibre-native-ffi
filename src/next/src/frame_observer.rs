// Ports maplibre-native-ffi's SessionFrameObserver
// (src/render/render_session_common.hpp): the renderer observer of a render
// session whose renderer lives on the runtime thread (session.rs,
// metal_surface.rs). It forwards to the map's renderer observer and keeps
// what a frame result reports of the frame.

use crate::types::*;
use maplibre_native_next::mln_renderer::{RenderMode, RendererObserver};
use maplibre_native_next::mln_util::exception::ExceptionPtr;
use std::cell::{Cell, RefCell};
use std::rc::{Rc, Weak};

/// A session's renderer observer: forwards to the map's renderer observer,
/// records how the last frame finished (its frame result's mode and
/// needs_repaint), and holds back the frame callbacks of a render that the
/// host never sees (C++ `SessionFrameObserver`).
#[derive(Default)]
pub(crate) struct FrameObserver {
    delegate: RefCell<Option<Weak<dyn RendererObserver>>>,
    suppressed: Cell<bool>,
    finished: Cell<Option<(RenderMode, bool)>>,
}

impl FrameObserver {
    pub(crate) fn set_delegate(&self, delegate: Weak<dyn RendererObserver>) {
        *self.delegate.borrow_mut() = Some(delegate);
    }

    /// Starts a frame: `finished` reports how it finished.
    pub(crate) fn begin(&self) {
        self.finished.set(None);
    }

    /// The finished frame's mode and whether it needs another, or `None`
    /// when the render drew no frame.
    pub(crate) fn finished(&self) -> Option<(RenderMode, bool)> {
        self.finished.get()
    }

    /// Holds back the frame callbacks while `suppressed`.
    pub(crate) fn suppress_frame_callbacks(&self, suppressed: bool) {
        self.suppressed.set(suppressed);
    }

    fn delegate(&self) -> Option<Rc<dyn RendererObserver>> {
        self.delegate.borrow().as_ref().and_then(Weak::upgrade)
    }

    fn frame_delegate(&self) -> Option<Rc<dyn RendererObserver>> {
        if self.suppressed.get() {
            None
        } else {
            self.delegate()
        }
    }
}

impl RendererObserver for FrameObserver {
    fn on_invalidate(&self) {
        if let Some(delegate) = self.delegate() {
            delegate.on_invalidate();
        }
    }

    fn on_resource_error(&self, error: ExceptionPtr) {
        if let Some(delegate) = self.delegate() {
            delegate.on_resource_error(error);
        }
    }

    fn on_will_start_rendering_map(&self) {
        if let Some(delegate) = self.frame_delegate() {
            delegate.on_will_start_rendering_map();
        }
    }

    fn on_will_start_rendering_frame(&self) {
        if let Some(delegate) = self.frame_delegate() {
            delegate.on_will_start_rendering_frame();
        }
    }

    fn on_did_finish_rendering_frame(
        &self,
        mode: RenderMode,
        repaint: bool,
        placement_changed: bool,
    ) {
        self.finished.set(Some((mode, repaint)));
        if let Some(delegate) = self.frame_delegate() {
            delegate.on_did_finish_rendering_frame(mode, repaint, placement_changed);
        }
    }

    fn on_did_finish_rendering_map(&self) {
        if let Some(delegate) = self.frame_delegate() {
            delegate.on_did_finish_rendering_map();
        }
    }

    fn on_style_image_missing(&self, id: &str, done: &mut dyn FnMut()) {
        match self.delegate() {
            Some(delegate) => delegate.on_style_image_missing(id, done),
            None => done(),
        }
    }

    fn on_remove_unused_style_images(&self, ids: &[String]) {
        if let Some(delegate) = self.delegate() {
            delegate.on_remove_unused_style_images(ids);
        }
    }

    fn on_render_error(&self, error: ExceptionPtr) {
        if let Some(delegate) = self.delegate() {
            delegate.on_render_error(error);
        }
    }
}

pub(crate) fn render_mode_code(mode: RenderMode) -> u32 {
    match mode {
        RenderMode::Partial => MLN_RENDER_MODE_PARTIAL,
        RenderMode::Full => MLN_RENDER_MODE_FULL,
    }
}
