use std::ffi::CString;
use std::io::Cursor;
use std::os::raw::c_char;
use std::ptr;
use std::slice;

use image::{ColorType, ImageDecoder};

const MAX_IMAGE_DIMENSION: u32 = 16_384;
const MAX_IMAGE_ALLOC_BYTES: u64 = 512 * 1024 * 1024;

#[repr(C)]
pub struct MlnRustDecodedImage {
    pub width: u32,
    pub height: u32,
    pub data: *mut u8,
    pub data_len: usize,
    pub error: *mut c_char,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_decode_image(
    data: *const u8,
    data_len: usize,
) -> MlnRustDecodedImage {
    if data.is_null() {
        return decode_error("image input pointer is null");
    }

    // SAFETY: The C++ caller passes a pointer/length pair valid for this call.
    let encoded = unsafe { slice::from_raw_parts(data, data_len) };
    match decode_image(encoded) {
        Ok(decoded) => decoded,
        Err(message) => decode_error(&message),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_decoded_image_free(image: MlnRustDecodedImage) {
    if !image.data.is_null() && image.data_len > 0 {
        // SAFETY: `mlnffi_rust_decode_image` returns `data` from `Box<[u8]>`,
        // and ownership is transferred back here with the original length.
        unsafe {
            drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                image.data,
                image.data_len,
            )));
        }
    }

    if !image.error.is_null() {
        // SAFETY: `decode_error` returns `error` from `CString::into_raw`.
        unsafe {
            drop(CString::from_raw(image.error));
        }
    }
}

fn decode_image(encoded: &[u8]) -> Result<MlnRustDecodedImage, String> {
    let mut reader = image::ImageReader::new(Cursor::new(encoded))
        .with_guessed_format()
        .map_err(|error| error.to_string())?;
    let mut limits = image::Limits::default();
    limits.max_image_width = Some(MAX_IMAGE_DIMENSION);
    limits.max_image_height = Some(MAX_IMAGE_DIMENSION);
    limits.max_alloc = Some(MAX_IMAGE_ALLOC_BYTES);
    reader.limits(limits.clone());

    let mut decoder = reader.into_decoder().map_err(|error| error.to_string())?;
    let (width, height) = decoder.dimensions();
    let color_type = decoder.color_type();
    let decoded_len = decoder.total_bytes();
    limits
        .reserve(decoded_len)
        .and_then(|()| decoder.set_limits(limits))
        .map_err(|error| error.to_string())?;
    let too_large = || "decoded image is too large".to_owned();
    let decoded_len = usize::try_from(decoded_len).map_err(|_| too_large())?;
    let expanded_len = u64::from(width)
        .checked_mul(u64::from(height))
        .and_then(|pixels| pixels.checked_mul(4))
        .ok_or_else(too_large)?;
    if expanded_len > MAX_IMAGE_ALLOC_BYTES {
        return Err("decoded image exceeds allocation limit".to_owned());
    }
    let expanded_len = usize::try_from(expanded_len).map_err(|_| too_large())?;

    // The decoded and expanded buffers are as large as the image. Allocate
    // them fallibly, so running out of memory fails this image instead of
    // aborting the process.
    let mut decoded = try_vec_with_capacity(decoded_len)?;
    decoded.resize(decoded_len, 0);
    decoder
        .read_image(&mut decoded)
        .map_err(|error| error.to_string())?;
    let mut data = expand_to_rgba8(decoded, color_type, expanded_len)?;

    premultiply_rgba(&mut data);

    let data = data.into_boxed_slice();
    let data_len = data.len();
    let data_ptr = Box::into_raw(data) as *mut u8;

    Ok(MlnRustDecodedImage {
        width,
        height,
        data: data_ptr,
        data_len,
        error: ptr::null_mut(),
    })
}

fn try_vec_with_capacity(capacity: usize) -> Result<Vec<u8>, String> {
    let mut data = Vec::new();
    data.try_reserve_exact(capacity)
        .map_err(|_| "out of memory decoding image".to_owned())?;
    Ok(data)
}

/// Converts decoded samples to RGBA8 as `DynamicImage::to_rgba8` does. Samples
/// that are already RGBA8 keep their buffer.
fn expand_to_rgba8(
    mut data: Vec<u8>,
    color_type: ColorType,
    expanded_len: usize,
) -> Result<Vec<u8>, String> {
    if color_type == ColorType::Rgba8 {
        return Ok(data);
    }

    let (samples, color_type) = match color_type {
        ColorType::L16 | ColorType::La16 | ColorType::Rgb16 | ColorType::Rgba16 => {
            // Narrow the native-endian 16-bit samples in place, rounding as
            // the image crate does.
            let len = data.len() / 2;
            for index in 0..len {
                let sample = u16::from_ne_bytes([data[2 * index], data[2 * index + 1]]);
                data[index] = ((u32::from(sample) + 128) / 257) as u8;
            }
            let color_type = match color_type {
                ColorType::L16 => ColorType::L8,
                ColorType::La16 => ColorType::La8,
                ColorType::Rgb16 => ColorType::Rgb8,
                _ => ColorType::Rgba8,
            };
            (&data[..len], color_type)
        }
        other => (&data[..], other),
    };

    let mut rgba = try_vec_with_capacity(expanded_len)?;
    match color_type {
        ColorType::Rgba8 => rgba.extend_from_slice(samples),
        ColorType::Rgb8 => rgba.extend(
            samples
                .as_chunks::<3>()
                .0
                .iter()
                .flat_map(|&[r, g, b]| [r, g, b, u8::MAX]),
        ),
        ColorType::La8 => rgba.extend(
            samples
                .as_chunks::<2>()
                .0
                .iter()
                .flat_map(|&[l, a]| [l, l, l, a]),
        ),
        ColorType::L8 => rgba.extend(samples.iter().flat_map(|&l| [l, l, l, u8::MAX])),
        _ => return Err(format!("unsupported decoded color type {color_type:?}")),
    }
    Ok(rgba)
}

fn premultiply_rgba(data: &mut [u8]) {
    for pixel in data.as_chunks_mut::<4>().0 {
        let alpha = u16::from(pixel[3]);
        pixel[0] = premultiply_channel(pixel[0], alpha);
        pixel[1] = premultiply_channel(pixel[1], alpha);
        pixel[2] = premultiply_channel(pixel[2], alpha);
    }
}

fn premultiply_channel(channel: u8, alpha: u16) -> u8 {
    ((u16::from(channel) * alpha + 127) / 255) as u8
}

fn decode_error(message: &str) -> MlnRustDecodedImage {
    let error = match CString::new(message) {
        Ok(error) => error.into_raw(),
        Err(_) => CString::new("image decode failed")
            .expect("static string has no interior nul")
            .into_raw(),
    };

    MlnRustDecodedImage {
        width: 0,
        height: 0,
        data: ptr::null_mut(),
        data_len: 0,
        error,
    }
}

#[cfg(test)]
mod tests {
    use std::alloc::{GlobalAlloc, Layout, System};
    use std::cell::Cell;
    use std::io::Cursor;

    use super::*;

    thread_local! {
        static MAX_ALLOCATION: Cell<usize> = const { Cell::new(usize::MAX) };
    }

    /// Fails allocations on this thread that exceed `MAX_ALLOCATION`.
    struct LimitedAllocator;

    // SAFETY: Every allocation either fails or is forwarded to `System`.
    unsafe impl GlobalAlloc for LimitedAllocator {
        unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
            if layout.size() > MAX_ALLOCATION.get() {
                return ptr::null_mut();
            }
            // SAFETY: The caller upholds `GlobalAlloc::alloc`'s contract.
            unsafe { System.alloc(layout) }
        }

        unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
            // SAFETY: `ptr` came from `System.alloc` with `layout`.
            unsafe { System.dealloc(ptr, layout) }
        }
    }

    #[global_allocator]
    static ALLOCATOR: LimitedAllocator = LimitedAllocator;

    #[test]
    fn failed_allocation_fails_only_the_image() {
        let (width, height) = (512, 512);
        let mut png = Vec::new();
        image::RgbImage::new(width, height)
            .write_to(&mut Cursor::new(&mut png), image::ImageFormat::Png)
            .unwrap();
        let decoded_len = (width * height * 3) as usize;
        let expanded_len = (width * height * 4) as usize;

        // The first limit fails the decoded buffer and the second fails the
        // RGBA expansion.
        for max_allocation in [decoded_len - 1, expanded_len - 1] {
            MAX_ALLOCATION.set(max_allocation);
            let result = decode_image(&png);
            MAX_ALLOCATION.set(usize::MAX);
            assert_eq!(
                result.err().as_deref(),
                Some("out of memory decoding image")
            );
        }

        let decoded = decode_image(&png).unwrap();
        assert_eq!((decoded.width, decoded.height), (width, height));
        // SAFETY: `decoded` came from `decode_image` and is freed once.
        unsafe { mlnffi_rust_decoded_image_free(decoded) };
    }
}
