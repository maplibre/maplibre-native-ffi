pub use maplibre_native_core::values::*;

pub(crate) trait NativeValue: Sized {
    type Raw;

    fn to_native(self) -> Self::Raw;
    fn from_native(value: Self::Raw) -> Self;
}

impl NativeValue for LatLng {
    type Raw = crate::sys::mln_lat_lng;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::lat_lng_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::lat_lng_from_native(value)
    }
}

impl NativeValue for LatLngBounds {
    type Raw = crate::sys::mln_lat_lng_bounds;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::lat_lng_bounds_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::lat_lng_bounds_from_native(value)
    }
}

impl NativeValue for ProjectedMeters {
    type Raw = crate::sys::mln_projected_meters;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::projected_meters_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::projected_meters_from_native(value)
    }
}

impl NativeValue for ScreenPoint {
    type Raw = crate::sys::mln_screen_point;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::screen_point_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::screen_point_from_native(value)
    }
}

impl NativeValue for ScreenBox {
    type Raw = crate::sys::mln_screen_box;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::screen_box_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        Self {
            min: ScreenPoint::from_native(value.min),
            max: ScreenPoint::from_native(value.max),
        }
    }
}

impl NativeValue for EdgeInsets {
    type Raw = crate::sys::mln_edge_insets;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::edge_insets_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::edge_insets_from_native(value)
    }
}

impl NativeValue for Vec3 {
    type Raw = crate::sys::mln_vec3;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::vec3_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::vec3_from_native(value)
    }
}

impl NativeValue for Quaternion {
    type Raw = crate::sys::mln_quaternion;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::quaternion_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        maplibre_native_core::values::quaternion_from_native(value)
    }
}

impl NativeValue for UnitBezier {
    type Raw = crate::sys::mln_unit_bezier;

    fn to_native(self) -> Self::Raw {
        maplibre_native_core::values::unit_bezier_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        Self {
            x1: value.x1,
            y1: value.y1,
            x2: value.x2,
            y2: value.y2,
        }
    }
}
