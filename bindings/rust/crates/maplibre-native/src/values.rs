use maplibre_native_sys as sys;

/// Geographic coordinate in degrees.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LatLng {
    pub latitude: f64,
    pub longitude: f64,
}

impl LatLng {
    pub const fn new(latitude: f64, longitude: f64) -> Self {
        Self {
            latitude,
            longitude,
        }
    }

    pub(crate) fn to_native(self) -> sys::mln_lat_lng {
        sys::mln_lat_lng {
            latitude: self.latitude,
            longitude: self.longitude,
        }
    }

    pub(crate) fn from_native(value: sys::mln_lat_lng) -> Self {
        Self {
            latitude: value.latitude,
            longitude: value.longitude,
        }
    }
}

/// Geographic bounds in degrees.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LatLngBounds {
    pub southwest: LatLng,
    pub northeast: LatLng,
}

impl LatLngBounds {
    pub const fn new(southwest: LatLng, northeast: LatLng) -> Self {
        Self {
            southwest,
            northeast,
        }
    }

    pub(crate) fn to_native(self) -> sys::mln_lat_lng_bounds {
        sys::mln_lat_lng_bounds {
            southwest: self.southwest.to_native(),
            northeast: self.northeast.to_native(),
        }
    }

    pub(crate) fn from_native(value: sys::mln_lat_lng_bounds) -> Self {
        Self {
            southwest: LatLng::from_native(value.southwest),
            northeast: LatLng::from_native(value.northeast),
        }
    }
}

/// Lower-level Spherical Mercator projected-meter coordinate.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ProjectedMeters {
    pub northing: f64,
    pub easting: f64,
}

impl ProjectedMeters {
    pub const fn new(northing: f64, easting: f64) -> Self {
        Self { northing, easting }
    }

    pub(crate) fn to_native(self) -> sys::mln_projected_meters {
        sys::mln_projected_meters {
            northing: self.northing,
            easting: self.easting,
        }
    }

    pub(crate) fn from_native(value: sys::mln_projected_meters) -> Self {
        Self {
            northing: value.northing,
            easting: value.easting,
        }
    }
}

/// Screen-space point in logical map pixels.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ScreenPoint {
    pub x: f64,
    pub y: f64,
}

impl ScreenPoint {
    pub const fn new(x: f64, y: f64) -> Self {
        Self { x, y }
    }

    pub(crate) fn to_native(self) -> sys::mln_screen_point {
        sys::mln_screen_point {
            x: self.x,
            y: self.y,
        }
    }

    pub(crate) fn from_native(value: sys::mln_screen_point) -> Self {
        Self {
            x: value.x,
            y: value.y,
        }
    }
}

/// Screen-space box in logical map pixels.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ScreenBox {
    pub min: ScreenPoint,
    pub max: ScreenPoint,
}

impl ScreenBox {
    pub const fn new(min: ScreenPoint, max: ScreenPoint) -> Self {
        Self { min, max }
    }
}

/// Screen-space inset in logical map pixels.
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct EdgeInsets {
    pub top: f64,
    pub left: f64,
    pub bottom: f64,
    pub right: f64,
}

impl EdgeInsets {
    pub const fn new(top: f64, left: f64, bottom: f64, right: f64) -> Self {
        Self {
            top,
            left,
            bottom,
            right,
        }
    }

    pub(crate) fn to_native(self) -> sys::mln_edge_insets {
        sys::mln_edge_insets {
            top: self.top,
            left: self.left,
            bottom: self.bottom,
            right: self.right,
        }
    }

    pub(crate) fn from_native(value: sys::mln_edge_insets) -> Self {
        Self {
            top: value.top,
            left: value.left,
            bottom: value.bottom,
            right: value.right,
        }
    }
}

/// Three-component vector used by free camera options.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Vec3 {
    pub const fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    pub(crate) fn to_native(self) -> sys::mln_vec3 {
        sys::mln_vec3 {
            x: self.x,
            y: self.y,
            z: self.z,
        }
    }

    pub(crate) fn from_native(value: sys::mln_vec3) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

/// Quaternion stored as x, y, z, w components.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Quaternion {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

impl Quaternion {
    pub const fn new(x: f64, y: f64, z: f64, w: f64) -> Self {
        Self { x, y, z, w }
    }

    pub(crate) fn to_native(self) -> sys::mln_quaternion {
        sys::mln_quaternion {
            x: self.x,
            y: self.y,
            z: self.z,
            w: self.w,
        }
    }

    pub(crate) fn from_native(value: sys::mln_quaternion) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
            w: value.w,
        }
    }
}

/// Cubic easing curve for animated camera transitions.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct UnitBezier {
    pub x1: f64,
    pub y1: f64,
    pub x2: f64,
    pub y2: f64,
}

impl UnitBezier {
    pub const fn new(x1: f64, y1: f64, x2: f64, y2: f64) -> Self {
        Self { x1, y1, x2, y2 }
    }

    pub(crate) fn to_native(self) -> sys::mln_unit_bezier {
        sys::mln_unit_bezier {
            x1: self.x1,
            y1: self.y1,
            x2: self.x2,
            y2: self.y2,
        }
    }

    #[cfg(test)]
    pub(crate) fn from_native(value: sys::mln_unit_bezier) -> Self {
        Self {
            x1: value.x1,
            y1: value.y1,
            x2: value.x2,
            y2: value.y2,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn value_types_round_trip_native_layouts() {
        let coordinate = LatLng::new(12.5, -45.0);
        assert_eq!(LatLng::from_native(coordinate.to_native()), coordinate);

        let bounds = LatLngBounds::new(LatLng::new(-1.0, -2.0), LatLng::new(3.0, 4.0));
        assert_eq!(LatLngBounds::from_native(bounds.to_native()), bounds);

        let meters = ProjectedMeters::new(1.25, 2.5);
        assert_eq!(ProjectedMeters::from_native(meters.to_native()), meters);

        let point = ScreenPoint::new(8.0, 9.0);
        assert_eq!(ScreenPoint::from_native(point.to_native()), point);

        let insets = EdgeInsets::new(1.0, 2.0, 3.0, 4.0);
        assert_eq!(EdgeInsets::from_native(insets.to_native()), insets);

        let vector = Vec3::new(1.0, 2.0, 3.0);
        assert_eq!(Vec3::from_native(vector.to_native()), vector);

        let quaternion = Quaternion::new(1.0, 2.0, 3.0, 4.0);
        assert_eq!(Quaternion::from_native(quaternion.to_native()), quaternion);

        let easing = UnitBezier::new(0.0, 0.25, 0.75, 1.0);
        assert_eq!(UnitBezier::from_native(easing.to_native()), easing);

        let screen_box = ScreenBox::new(ScreenPoint::new(1.0, 2.0), ScreenPoint::new(3.0, 4.0));
        assert_eq!(screen_box.min.x, 1.0);
        assert_eq!(screen_box.max.y, 4.0);
    }
}
