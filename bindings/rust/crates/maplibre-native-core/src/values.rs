use maplibre_native_sys as sys;

/// Internal helper trait for Rust adapter code that needs raw value conversion.
///
/// Public language bindings can use the free functions in this module instead
/// when a trait would expose more raw ABI detail than their API should show.
#[doc(hidden)]
pub trait NativeValue: Sized {
    type Raw;

    fn to_native(self) -> Self::Raw;
    fn from_native(value: Self::Raw) -> Self;
}

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

    fn to_native(self) -> sys::mln_lat_lng {
        sys::mln_lat_lng {
            latitude: self.latitude,
            longitude: self.longitude,
        }
    }

    fn from_native(value: sys::mln_lat_lng) -> Self {
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

    fn to_native(self) -> sys::mln_lat_lng_bounds {
        sys::mln_lat_lng_bounds {
            southwest: self.southwest.to_native(),
            northeast: self.northeast.to_native(),
        }
    }

    fn from_native(value: sys::mln_lat_lng_bounds) -> Self {
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

    fn to_native(self) -> sys::mln_projected_meters {
        sys::mln_projected_meters {
            northing: self.northing,
            easting: self.easting,
        }
    }

    fn from_native(value: sys::mln_projected_meters) -> Self {
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

    fn to_native(self) -> sys::mln_screen_point {
        sys::mln_screen_point {
            x: self.x,
            y: self.y,
        }
    }

    fn from_native(value: sys::mln_screen_point) -> Self {
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

    fn to_native(self) -> sys::mln_edge_insets {
        sys::mln_edge_insets {
            top: self.top,
            left: self.left,
            bottom: self.bottom,
            right: self.right,
        }
    }

    fn from_native(value: sys::mln_edge_insets) -> Self {
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

    fn to_native(self) -> sys::mln_vec3 {
        sys::mln_vec3 {
            x: self.x,
            y: self.y,
            z: self.z,
        }
    }

    fn from_native(value: sys::mln_vec3) -> Self {
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

    fn to_native(self) -> sys::mln_quaternion {
        sys::mln_quaternion {
            x: self.x,
            y: self.y,
            z: self.z,
            w: self.w,
        }
    }

    fn from_native(value: sys::mln_quaternion) -> Self {
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

    fn to_native(self) -> sys::mln_unit_bezier {
        sys::mln_unit_bezier {
            x1: self.x1,
            y1: self.y1,
            x2: self.x2,
            y2: self.y2,
        }
    }
}

pub fn lat_lng_to_native(value: LatLng) -> sys::mln_lat_lng {
    value.to_native()
}

pub fn lat_lng_from_native(value: sys::mln_lat_lng) -> LatLng {
    LatLng::from_native(value)
}

pub fn lat_lng_bounds_to_native(value: LatLngBounds) -> sys::mln_lat_lng_bounds {
    value.to_native()
}

pub fn lat_lng_bounds_from_native(value: sys::mln_lat_lng_bounds) -> LatLngBounds {
    LatLngBounds::from_native(value)
}

pub fn projected_meters_to_native(value: ProjectedMeters) -> sys::mln_projected_meters {
    value.to_native()
}

pub fn projected_meters_from_native(value: sys::mln_projected_meters) -> ProjectedMeters {
    ProjectedMeters::from_native(value)
}

pub fn screen_point_to_native(value: ScreenPoint) -> sys::mln_screen_point {
    value.to_native()
}

pub fn screen_point_from_native(value: sys::mln_screen_point) -> ScreenPoint {
    ScreenPoint::from_native(value)
}

pub fn empty_lat_lng() -> sys::mln_lat_lng {
    LatLng::new(0.0, 0.0).to_native()
}

pub fn empty_screen_point() -> sys::mln_screen_point {
    ScreenPoint::new(0.0, 0.0).to_native()
}

pub fn empty_lat_lng_bounds() -> sys::mln_lat_lng_bounds {
    LatLngBounds::new(LatLng::new(0.0, 0.0), LatLng::new(0.0, 0.0)).to_native()
}

pub fn lat_lngs_to_native(coordinates: &[LatLng]) -> Vec<sys::mln_lat_lng> {
    coordinates.iter().copied().map(LatLng::to_native).collect()
}

pub fn screen_points_to_native(points: &[ScreenPoint]) -> Vec<sys::mln_screen_point> {
    points.iter().copied().map(ScreenPoint::to_native).collect()
}

pub fn screen_box_to_native(value: ScreenBox) -> sys::mln_screen_box {
    sys::mln_screen_box {
        min: value.min.to_native(),
        max: value.max.to_native(),
    }
}

pub fn edge_insets_to_native(value: EdgeInsets) -> sys::mln_edge_insets {
    value.to_native()
}

pub fn edge_insets_from_native(value: sys::mln_edge_insets) -> EdgeInsets {
    EdgeInsets::from_native(value)
}

pub fn vec3_to_native(value: Vec3) -> sys::mln_vec3 {
    value.to_native()
}

pub fn vec3_from_native(value: sys::mln_vec3) -> Vec3 {
    Vec3::from_native(value)
}

pub fn quaternion_to_native(value: Quaternion) -> sys::mln_quaternion {
    value.to_native()
}

pub fn quaternion_from_native(value: sys::mln_quaternion) -> Quaternion {
    Quaternion::from_native(value)
}

pub fn unit_bezier_to_native(value: UnitBezier) -> sys::mln_unit_bezier {
    value.to_native()
}

pub fn unit_bezier_from_native(value: sys::mln_unit_bezier) -> UnitBezier {
    UnitBezier {
        x1: value.x1,
        y1: value.y1,
        x2: value.x2,
        y2: value.y2,
    }
}

impl NativeValue for LatLng {
    type Raw = sys::mln_lat_lng;

    fn to_native(self) -> Self::Raw {
        LatLng::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        LatLng::from_native(value)
    }
}

impl NativeValue for LatLngBounds {
    type Raw = sys::mln_lat_lng_bounds;

    fn to_native(self) -> Self::Raw {
        LatLngBounds::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        LatLngBounds::from_native(value)
    }
}

impl NativeValue for ProjectedMeters {
    type Raw = sys::mln_projected_meters;

    fn to_native(self) -> Self::Raw {
        ProjectedMeters::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        ProjectedMeters::from_native(value)
    }
}

impl NativeValue for ScreenPoint {
    type Raw = sys::mln_screen_point;

    fn to_native(self) -> Self::Raw {
        ScreenPoint::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        ScreenPoint::from_native(value)
    }
}

impl NativeValue for ScreenBox {
    type Raw = sys::mln_screen_box;

    fn to_native(self) -> Self::Raw {
        screen_box_to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        Self {
            min: ScreenPoint::from_native(value.min),
            max: ScreenPoint::from_native(value.max),
        }
    }
}

impl NativeValue for EdgeInsets {
    type Raw = sys::mln_edge_insets;

    fn to_native(self) -> Self::Raw {
        EdgeInsets::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        EdgeInsets::from_native(value)
    }
}

impl NativeValue for Vec3 {
    type Raw = sys::mln_vec3;

    fn to_native(self) -> Self::Raw {
        Vec3::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        Vec3::from_native(value)
    }
}

impl NativeValue for Quaternion {
    type Raw = sys::mln_quaternion;

    fn to_native(self) -> Self::Raw {
        Quaternion::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        Quaternion::from_native(value)
    }
}

impl NativeValue for UnitBezier {
    type Raw = sys::mln_unit_bezier;

    fn to_native(self) -> Self::Raw {
        UnitBezier::to_native(self)
    }

    fn from_native(value: Self::Raw) -> Self {
        unit_bezier_from_native(value)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub struct TextureImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}

impl TextureImageInfo {
    pub const fn new(width: u32, height: u32, stride: u32, byte_length: usize) -> Self {
        Self {
            width,
            height,
            stride,
            byte_length,
        }
    }
}

pub fn texture_image_info_from_native(raw: &sys::mln_texture_image_info) -> TextureImageInfo {
    TextureImageInfo {
        width: raw.width,
        height: raw.height,
        stride: raw.stride,
        byte_length: raw.byte_length,
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct PremultipliedRgba8Image {
    pub info: TextureImageInfo,
    pub data: Vec<u8>,
}

impl PremultipliedRgba8Image {
    pub fn new(info: TextureImageInfo, data: Vec<u8>) -> Self {
        Self { info, data }
    }
}

pub fn premultiplied_rgba8_image_to_native(
    image: &PremultipliedRgba8Image,
) -> sys::mln_premultiplied_rgba8_image {
    let mut raw = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    raw.width = image.info.width;
    raw.height = image.info.height;
    raw.stride = image.info.stride;
    raw.pixels = image.data.as_ptr();
    raw.byte_length = image.data.len();
    raw
}

/// Copied fixed metadata for one runtime style image.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
pub struct StyleImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
    pub pixel_ratio: f32,
    pub sdf: bool,
}

pub fn style_image_info_from_native(raw: &sys::mln_style_image_info) -> StyleImageInfo {
    StyleImageInfo {
        width: raw.width,
        height: raw.height,
        stride: raw.stride,
        byte_length: raw.byte_length,
        pixel_ratio: raw.pixel_ratio,
        sdf: raw.sdf,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn premultiplied_rgba8_image_materializes_raw_view() {
        let image = PremultipliedRgba8Image {
            info: TextureImageInfo {
                width: 2,
                height: 1,
                stride: 8,
                byte_length: 8,
            },
            data: vec![1, 2, 3, 4, 5, 6, 7, 8],
        };
        let raw = premultiplied_rgba8_image_to_native(&image);
        assert_eq!(
            raw.size,
            std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as u32
        );
        assert_eq!(raw.width, 2);
        assert_eq!(raw.height, 1);
        assert_eq!(raw.stride, 8);
        assert_eq!(raw.pixels, image.data.as_ptr());
        assert_eq!(raw.byte_length, image.data.len());
    }
}
