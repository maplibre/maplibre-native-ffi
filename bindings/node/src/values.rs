use maplibre_native_core as core;
use maplibre_native_sys as sys;
use napi::bindgen_prelude::Result;
use napi_derive::napi;

use crate::error;

#[napi(object)]
pub struct LatLng {
    pub latitude: f64,
    pub longitude: f64,
}

#[napi(object)]
pub struct LatLngBounds {
    pub southwest: LatLng,
    pub northeast: LatLng,
}

#[napi(object)]
pub struct ScreenPoint {
    pub x: f64,
    pub y: f64,
}

#[napi(object)]
pub struct ProjectedMeters {
    pub northing: f64,
    pub easting: f64,
}

#[napi(js_name = "nativeProjectedMetersForLatLng")]
pub fn native_projected_meters_for_lat_lng(coordinate: LatLng) -> Result<ProjectedMeters> {
    let mut raw_meters = sys::mln_projected_meters {
        northing: 0.0,
        easting: 0.0,
    };
    core::check(unsafe {
        sys::mln_projected_meters_for_lat_lng(coordinate.into_native(), &mut raw_meters)
    })
    .map_err(error::from_core)?;
    Ok(ProjectedMeters::from_native(raw_meters))
}

#[napi(js_name = "nativeLatLngForProjectedMeters")]
pub fn native_lat_lng_for_projected_meters(meters: ProjectedMeters) -> Result<LatLng> {
    let mut raw_coordinate = sys::mln_lat_lng {
        latitude: 0.0,
        longitude: 0.0,
    };
    core::check(unsafe {
        sys::mln_lat_lng_for_projected_meters(meters.into_native(), &mut raw_coordinate)
    })
    .map_err(error::from_core)?;
    Ok(LatLng::from_native(raw_coordinate))
}

impl LatLng {
    pub(crate) fn into_core(self) -> core::LatLng {
        core::LatLng::new(self.latitude, self.longitude)
    }

    pub(crate) fn from_core(value: core::LatLng) -> Self {
        Self {
            latitude: value.latitude,
            longitude: value.longitude,
        }
    }

    pub(crate) fn into_native(self) -> sys::mln_lat_lng {
        core::values::lat_lng_to_native(self.into_core())
    }

    pub(crate) fn from_native(raw: sys::mln_lat_lng) -> Self {
        Self::from_core(core::values::lat_lng_from_native(raw))
    }
}

impl LatLngBounds {
    pub(crate) fn into_core(self) -> core::LatLngBounds {
        core::LatLngBounds::new(self.southwest.into_core(), self.northeast.into_core())
    }

    pub(crate) fn from_core(value: core::LatLngBounds) -> Self {
        Self {
            southwest: LatLng::from_core(value.southwest),
            northeast: LatLng::from_core(value.northeast),
        }
    }
}

impl ScreenPoint {
    pub(crate) fn into_core(self) -> core::ScreenPoint {
        core::ScreenPoint::new(self.x, self.y)
    }

    pub(crate) fn from_core(value: core::ScreenPoint) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
    }

    pub(crate) fn into_native(self) -> sys::mln_screen_point {
        core::values::screen_point_to_native(self.into_core())
    }

    pub(crate) fn from_native(raw: sys::mln_screen_point) -> Self {
        Self::from_core(core::values::screen_point_from_native(raw))
    }
}

impl ProjectedMeters {
    fn into_native(self) -> sys::mln_projected_meters {
        core::values::projected_meters_to_native(core::ProjectedMeters::new(
            self.northing,
            self.easting,
        ))
    }

    fn from_native(raw: sys::mln_projected_meters) -> Self {
        let value = core::values::projected_meters_from_native(raw);
        Self {
            northing: value.northing,
            easting: value.easting,
        }
    }
}
