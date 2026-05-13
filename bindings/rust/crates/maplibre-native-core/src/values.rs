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
    sys::mln_premultiplied_rgba8_image {
        size: std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as u32,
        width: image.info.width,
        height: image.info.height,
        stride: image.info.stride,
        pixels: image.data.as_ptr(),
        byte_length: image.data.len(),
    }
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
    fn lat_lng_maps_raw_fields() {
        let value = LatLng::new(45.0, -122.0);
        let raw = lat_lng_to_native(value);
        assert_eq!(raw.latitude, 45.0);
        assert_eq!(raw.longitude, -122.0);

        let copied = lat_lng_from_native(sys::mln_lat_lng {
            latitude: 46.0,
            longitude: -121.0,
        });
        assert_eq!(copied, LatLng::new(46.0, -121.0));
    }

    #[test]
    fn lat_lng_bounds_maps_raw_fields() {
        let value = LatLngBounds::new(LatLng::new(45.0, -122.0), LatLng::new(46.0, -121.0));
        let raw = lat_lng_bounds_to_native(value);
        assert_eq!(raw.southwest.latitude, 45.0);
        assert_eq!(raw.southwest.longitude, -122.0);
        assert_eq!(raw.northeast.latitude, 46.0);
        assert_eq!(raw.northeast.longitude, -121.0);

        let copied = lat_lng_bounds_from_native(sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: 1.0,
                longitude: 2.0,
            },
            northeast: sys::mln_lat_lng {
                latitude: 3.0,
                longitude: 4.0,
            },
        });
        assert_eq!(
            copied,
            LatLngBounds::new(LatLng::new(1.0, 2.0), LatLng::new(3.0, 4.0))
        );
    }

    #[test]
    fn projected_meters_maps_raw_fields() {
        let value = ProjectedMeters::new(10.5, -20.25);
        let raw = projected_meters_to_native(value);
        assert_eq!(raw.northing, 10.5);
        assert_eq!(raw.easting, -20.25);

        let copied = projected_meters_from_native(sys::mln_projected_meters {
            northing: 1.5,
            easting: 2.5,
        });
        assert_eq!(copied, ProjectedMeters::new(1.5, 2.5));
    }

    #[test]
    fn screen_point_and_box_map_raw_fields() {
        let point = ScreenPoint::new(128.0, 256.0);
        let raw_point = screen_point_to_native(point);
        assert_eq!(raw_point.x, 128.0);
        assert_eq!(raw_point.y, 256.0);
        assert_eq!(
            screen_point_from_native(sys::mln_screen_point { x: 1.0, y: 2.0 }),
            ScreenPoint::new(1.0, 2.0)
        );

        let raw_box = screen_box_to_native(ScreenBox::new(
            ScreenPoint::new(1.0, 2.0),
            ScreenPoint::new(3.0, 4.0),
        ));
        assert_eq!(raw_box.min.x, 1.0);
        assert_eq!(raw_box.min.y, 2.0);
        assert_eq!(raw_box.max.x, 3.0);
        assert_eq!(raw_box.max.y, 4.0);
    }

    #[test]
    fn edge_insets_maps_raw_fields() {
        let value = EdgeInsets::new(1.0, 2.0, 3.0, 4.0);
        let raw = edge_insets_to_native(value);
        assert_eq!(raw.top, 1.0);
        assert_eq!(raw.left, 2.0);
        assert_eq!(raw.bottom, 3.0);
        assert_eq!(raw.right, 4.0);

        let copied = edge_insets_from_native(sys::mln_edge_insets {
            top: 5.0,
            left: 6.0,
            bottom: 7.0,
            right: 8.0,
        });
        assert_eq!(copied, EdgeInsets::new(5.0, 6.0, 7.0, 8.0));
    }

    #[test]
    fn vec3_maps_raw_fields() {
        let value = Vec3::new(1.0, 2.0, 3.0);
        let raw = vec3_to_native(value);
        assert_eq!(raw.x, 1.0);
        assert_eq!(raw.y, 2.0);
        assert_eq!(raw.z, 3.0);

        let copied = vec3_from_native(sys::mln_vec3 {
            x: 4.0,
            y: 5.0,
            z: 6.0,
        });
        assert_eq!(copied, Vec3::new(4.0, 5.0, 6.0));
    }

    #[test]
    fn quaternion_maps_raw_fields() {
        let value = Quaternion::new(1.0, 2.0, 3.0, 4.0);
        let raw = quaternion_to_native(value);
        assert_eq!(raw.x, 1.0);
        assert_eq!(raw.y, 2.0);
        assert_eq!(raw.z, 3.0);
        assert_eq!(raw.w, 4.0);

        let copied = quaternion_from_native(sys::mln_quaternion {
            x: 5.0,
            y: 6.0,
            z: 7.0,
            w: 8.0,
        });
        assert_eq!(copied, Quaternion::new(5.0, 6.0, 7.0, 8.0));
    }

    #[test]
    fn texture_image_info_maps_raw_fields() {
        let copied = texture_image_info_from_native(&sys::mln_texture_image_info {
            size: 0,
            width: 64,
            height: 32,
            stride: 256,
            byte_length: 8192,
        });
        assert_eq!(copied.width, 64);
        assert_eq!(copied.height, 32);
        assert_eq!(copied.stride, 256);
        assert_eq!(copied.byte_length, 8192);
    }

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

    #[test]
    fn style_image_info_maps_raw_fields() {
        let copied = style_image_info_from_native(&sys::mln_style_image_info {
            size: 0,
            width: 10,
            height: 11,
            stride: 40,
            byte_length: 440,
            pixel_ratio: 2.0,
            sdf: true,
        });
        assert_eq!(copied.width, 10);
        assert_eq!(copied.height, 11);
        assert_eq!(copied.stride, 40);
        assert_eq!(copied.byte_length, 440);
        assert_eq!(copied.pixel_ratio, 2.0);
        assert!(copied.sdf);
    }

    #[test]
    fn unit_bezier_materializes_raw_fields() {
        let value = UnitBezier::new(0.1, 0.2, 0.3, 0.4);
        let raw = unit_bezier_to_native(value);
        assert_eq!(raw.x1, 0.1);
        assert_eq!(raw.y1, 0.2);
        assert_eq!(raw.x2, 0.3);
        assert_eq!(raw.y2, 0.4);
    }
}
