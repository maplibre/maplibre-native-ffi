use std::ffi::CString;
use std::io::Cursor;
use std::os::raw::c_char;
use std::ptr;
use std::slice;

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
            drop(Box::from_raw(slice::from_raw_parts_mut(
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
    reader.limits(limits);

    let image = reader.decode().map_err(|error| error.to_string())?;
    let expanded_len = u64::from(image.width())
        .checked_mul(u64::from(image.height()))
        .and_then(|pixels| pixels.checked_mul(4))
        .ok_or_else(|| "decoded image is too large".to_owned())?;
    if expanded_len > MAX_IMAGE_ALLOC_BYTES {
        return Err("decoded image exceeds allocation limit".to_owned());
    }

    let rgba = image.to_rgba8();
    let (width, height) = rgba.dimensions();
    let mut data = rgba.into_raw();

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

fn premultiply_rgba(data: &mut [u8]) {
    for pixel in data.chunks_exact_mut(4) {
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
