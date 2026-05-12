use winit::dpi::PhysicalSize;
use winit::window::Window;

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Viewport {
    pub logical_width: u32,
    pub logical_height: u32,
    pub physical_width: u32,
    pub physical_height: u32,
    pub scale_factor: f64,
}

impl Viewport {
    pub fn from_window(window: &Window) -> Self {
        let physical = window.inner_size();
        Self::from_physical_size(physical, window.scale_factor())
    }

    pub fn from_physical_size(physical: PhysicalSize<u32>, scale_factor: f64) -> Self {
        let physical_width = physical.width;
        let physical_height = physical.height;
        let safe_scale = if scale_factor.is_finite() && scale_factor > 0.0 {
            scale_factor
        } else {
            1.0
        };
        Self {
            logical_width: scaled_logical_size(physical_width, safe_scale),
            logical_height: scaled_logical_size(physical_height, safe_scale),
            physical_width,
            physical_height,
            scale_factor: safe_scale,
        }
    }

    pub fn is_empty(self) -> bool {
        self.logical_width == 0
            || self.logical_height == 0
            || self.physical_width == 0
            || self.physical_height == 0
    }

    pub fn log(self, label: &str) {
        println!(
            "{label}: logical={}x{} physical={}x{} scale={:.2}",
            self.logical_width,
            self.logical_height,
            self.physical_width,
            self.physical_height,
            self.scale_factor
        );
    }
}

fn scaled_logical_size(physical_size: u32, scale_factor: f64) -> u32 {
    if physical_size == 0 {
        0
    } else {
        (f64::from(physical_size) / scale_factor).ceil().max(1.0) as u32
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn zero_physical_extent_stays_empty() {
        let viewport = Viewport::from_physical_size(PhysicalSize::new(0, 720), 2.0);

        assert!(viewport.is_empty());
        assert_eq!(viewport.physical_width, 0);
        assert_eq!(viewport.logical_width, 0);
        assert_eq!(viewport.physical_height, 720);
        assert_eq!(viewport.logical_height, 360);
    }

    #[test]
    fn nonzero_physical_extent_keeps_at_least_one_logical_pixel() {
        let viewport = Viewport::from_physical_size(PhysicalSize::new(1, 1), 2.0);

        assert!(!viewport.is_empty());
        assert_eq!(viewport.logical_width, 1);
        assert_eq!(viewport.logical_height, 1);
    }

    #[test]
    fn invalid_scale_factor_falls_back_to_one() {
        let viewport = Viewport::from_physical_size(PhysicalSize::new(100, 50), f64::NAN);

        assert_eq!(viewport.scale_factor, 1.0);
        assert_eq!(viewport.logical_width, 100);
        assert_eq!(viewport.logical_height, 50);
    }
}
