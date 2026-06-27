use maplibre_native::{Error, ErrorKind, RenderTargetExtent};

pub struct Viewport {
    pub logical_width: u32,
    pub logical_height: u32,
    pub scale_factor: f64,
}

impl Viewport {
    pub fn new(logical_width: u32, logical_height: u32, scale_factor: f64) -> Result<Self, Error> {
        if logical_width == 0
            || logical_height == 0
            || !scale_factor.is_finite()
            || scale_factor <= 0.0
        {
            return Err(Error::new(
                ErrorKind::InvalidArgument,
                None,
                "invalid browser viewport",
            ));
        }
        Ok(Self {
            logical_width,
            logical_height,
            scale_factor,
        })
    }

    pub fn extent(&self) -> RenderTargetExtent {
        RenderTargetExtent::new(self.logical_width, self.logical_height, self.scale_factor)
    }

    pub fn physical_width(&self) -> u32 {
        physical_dimension(self.logical_width, self.scale_factor)
    }

    pub fn physical_height(&self) -> u32 {
        physical_dimension(self.logical_height, self.scale_factor)
    }
}

fn physical_dimension(logical: u32, scale: f64) -> u32 {
    ((logical as f64 * scale).ceil().max(1.0)) as u32
}
