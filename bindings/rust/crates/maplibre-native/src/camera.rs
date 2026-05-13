pub use maplibre_native_core::camera::{
    AnimationOptions, BoundOptions, CameraFitOptions, CameraOptions, FreeCameraOptions,
    ProjectionMode,
};

use crate::sys;

pub(crate) trait CameraOptionsNativeExt {
    fn to_native(&self) -> sys::mln_camera_options;
    fn from_native(raw: sys::mln_camera_options) -> CameraOptions;
}

impl CameraOptionsNativeExt for CameraOptions {
    fn to_native(&self) -> sys::mln_camera_options {
        maplibre_native_core::camera::camera_options_to_native(self)
    }

    fn from_native(raw: sys::mln_camera_options) -> CameraOptions {
        maplibre_native_core::camera::camera_options_from_native(raw)
    }
}

pub(crate) trait AnimationOptionsNativeExt {
    fn to_native(&self) -> sys::mln_animation_options;
}

impl AnimationOptionsNativeExt for AnimationOptions {
    fn to_native(&self) -> sys::mln_animation_options {
        maplibre_native_core::camera::animation_options_to_native(self)
    }
}

pub(crate) trait CameraFitOptionsNativeExt {
    fn to_native(&self) -> sys::mln_camera_fit_options;
}

impl CameraFitOptionsNativeExt for CameraFitOptions {
    fn to_native(&self) -> sys::mln_camera_fit_options {
        maplibre_native_core::camera::camera_fit_options_to_native(self)
    }
}

pub(crate) trait BoundOptionsNativeExt {
    fn to_native(&self) -> sys::mln_bound_options;
    fn from_native(raw: sys::mln_bound_options) -> BoundOptions;
}

impl BoundOptionsNativeExt for BoundOptions {
    fn to_native(&self) -> sys::mln_bound_options {
        maplibre_native_core::camera::bound_options_to_native(self)
    }

    fn from_native(raw: sys::mln_bound_options) -> BoundOptions {
        maplibre_native_core::camera::bound_options_from_native(raw)
    }
}

pub(crate) trait FreeCameraOptionsNativeExt {
    fn to_native(&self) -> sys::mln_free_camera_options;
    fn from_native(raw: sys::mln_free_camera_options) -> FreeCameraOptions;
}

impl FreeCameraOptionsNativeExt for FreeCameraOptions {
    fn to_native(&self) -> sys::mln_free_camera_options {
        maplibre_native_core::camera::free_camera_options_to_native(self)
    }

    fn from_native(raw: sys::mln_free_camera_options) -> FreeCameraOptions {
        maplibre_native_core::camera::free_camera_options_from_native(raw)
    }
}

pub(crate) trait ProjectionModeNativeExt {
    fn to_native(&self) -> sys::mln_projection_mode;
    fn from_native(raw: sys::mln_projection_mode) -> ProjectionMode;
}

impl ProjectionModeNativeExt for ProjectionMode {
    fn to_native(&self) -> sys::mln_projection_mode {
        maplibre_native_core::camera::projection_mode_to_native(self)
    }

    fn from_native(raw: sys::mln_projection_mode) -> ProjectionMode {
        maplibre_native_core::camera::projection_mode_from_native(raw)
    }
}
