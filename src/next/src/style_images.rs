// Ports the style image half of maplibre-native-ffi src/c_api/style.cpp and
// src/map/style.cpp (validate_style_image_options, make_style_image,
// map_set_style_image, map_remove_style_image, map_get_style_image,
// style_image_info) onto the images of mln_style's `Style`.
//
// The images a host keeps while unused are the map observer's
// `kept_style_images` (map.rs), which a style load clears and which
// `on_can_remove_unused_style_image` reads (C++ `kept_style_images`).

use crate::delivery::{Lent, Value};
use crate::diagnostics::{Failure, boundary};
use crate::map::command;
use crate::style_sources::{non_empty_text, premultiplied_image, with_style};
use crate::types::*;
use maplibre_native_next::mln_style::image::{ImageContent, ImageStretches, Impl, TextFit};
use std::sync::Arc;

const KNOWN_OPTION_FIELDS: u32 = MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
    | MLN_STYLE_IMAGE_OPTION_SDF
    | MLN_STYLE_IMAGE_OPTION_STRETCH_X
    | MLN_STYLE_IMAGE_OPTION_STRETCH_Y
    | MLN_STYLE_IMAGE_OPTION_CONTENT
    | MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
    | MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
    | MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;

fn text_fit(value: mln_style_image_text_fit) -> TextFit {
    match value {
        MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY => TextFit::StretchOnly,
        MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL => TextFit::Proportional,
        _ => TextFit::StretchOrShrink,
    }
}

fn c_text_fit(value: Option<TextFit>) -> mln_style_image_text_fit {
    match value {
        Some(TextFit::StretchOnly) => MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY,
        Some(TextFit::Proportional) => MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL,
        _ => MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK,
    }
}

/// C++ `validate_style_image_options` over one axis's stretches: the copied
/// intervals.
///
/// # Safety
/// `stretches` is null or points at `count` readable `mln_image_stretch`es.
unsafe fn stretches(stretches: *const mln_image_stretch, count: usize, name: &str) -> Result<ImageStretches, Failure> {
    if count == 0 {
        return Ok(ImageStretches::new());
    }
    if stretches.is_null() {
        return Err(Failure::invalid_argument(format!("{name} must not be null when its count is non-zero")));
    }
    // SAFETY: the caller's contract.
    let stretches = unsafe { std::slice::from_raw_parts(stretches, count) };
    let mut previous_to = None;
    for stretch in stretches {
        if !(stretch.from.is_finite() && stretch.to.is_finite()) {
            return Err(Failure::invalid_argument(format!("{name} intervals must be finite")));
        }
        // MapLibre divides by the summed interval width, so an axis whose
        // intervals are all zero-width would divide by zero.
        if stretch.from >= stretch.to {
            return Err(Failure::invalid_argument(format!("{name} intervals must have a positive width")));
        }
        if previous_to.is_some_and(|to| stretch.from < to) {
            return Err(Failure::invalid_argument(format!("{name} intervals must increase and must not overlap")));
        }
        previous_to = Some(stretch.to);
    }
    Ok(stretches.iter().map(|stretch| (stretch.from, stretch.to)).collect())
}

/// A style image and whether it stays while unused: C++
/// `validate_style_image_input`, `make_style_image` and
/// `style_image_keeps_unused`.
///
/// # Safety
/// `image` and `options` as the C API contract has them (style.h).
unsafe fn make_style_image(
    id: String,
    image: *const mln_premultiplied_rgba8_image,
    options: *const mln_style_image_options,
) -> Result<(Impl, bool), Failure> {
    // SAFETY: the caller's contract.
    let pixels = unsafe { premultiplied_image(image) }?;
    let options = if options.is_null() {
        None
    } else {
        // SAFETY: the caller's contract.
        let Some(options) = (unsafe { read_sized(options) }) else {
            return Err(Failure::invalid_argument("mln_style_image_options.size is too small"));
        };
        Some(options)
    };
    let fields = options.map_or(0, |options| options.fields);
    if fields & !KNOWN_OPTION_FIELDS != 0 {
        return Err(Failure::invalid_argument("mln_style_image_options.fields contains unknown bits"));
    }
    let has = |field: u32| fields & field != 0;
    let mut pixel_ratio = 1.0f32;
    let mut sdf = false;
    let mut stretch_x = ImageStretches::new();
    let mut stretch_y = ImageStretches::new();
    let mut content = None;
    let mut text_fit_width = None;
    let mut text_fit_height = None;
    let mut keep_unused = false;
    if let Some(options) = options {
        if has(MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO) {
            if !(options.pixel_ratio.is_finite() && options.pixel_ratio > 0.0) {
                return Err(Failure::invalid_argument("pixel_ratio must be finite and positive"));
            }
            pixel_ratio = options.pixel_ratio;
        }
        if has(MLN_STYLE_IMAGE_OPTION_STRETCH_X) {
            // SAFETY: the caller's contract.
            stretch_x = unsafe { stretches(options.stretch_x, options.stretch_x_count, "stretch_x") }?;
        }
        if has(MLN_STYLE_IMAGE_OPTION_STRETCH_Y) {
            // SAFETY: the caller's contract.
            stretch_y = unsafe { stretches(options.stretch_y, options.stretch_y_count, "stretch_y") }?;
        }
        if has(MLN_STYLE_IMAGE_OPTION_CONTENT) {
            let mln_image_content { left, top, right, bottom } = options.content;
            if ![left, top, right, bottom].iter().all(|inset| inset.is_finite()) {
                return Err(Failure::invalid_argument("content insets must be finite"));
            }
            if left > right || top > bottom {
                return Err(Failure::invalid_argument("content insets must not run backwards"));
            }
            content = Some(ImageContent { left, top, right, bottom });
        }
        for (field, value, name, out) in [
            (MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH, options.text_fit_width, "text_fit_width", &mut text_fit_width),
            (MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT, options.text_fit_height, "text_fit_height", &mut text_fit_height),
        ] {
            if !has(field) {
                continue;
            }
            if value > MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL {
                return Err(Failure::invalid_argument(format!("{name} is invalid")));
            }
            *out = Some(text_fit(value));
        }
        if has(MLN_STYLE_IMAGE_OPTION_SDF) {
            sdf = options.sdf;
        }
        if has(MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED) {
            keep_unused = options.keep_unused;
        }
    }
    // C++ `style::Image`'s constructor throws `StyleImageException` (an
    // interval or content area outside the image), which the C boundary
    // reports as a native error.
    let image = Impl::new(id, pixels, pixel_ratio, sdf, stretch_x, stretch_y, content, text_fit_width, text_fit_height)
        .map_err(|error| Failure::new(MLN_STATUS_NATIVE_ERROR, error.to_string()))?;
    Ok((image, keep_unused))
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_set_style_image(
    map: mln_map,
    image_id: mln_buffer_view,
    image: *const mln_premultiplied_rgba8_image,
    options: *const mln_style_image_options,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(image_id, "image_id")?;
            let (image, keep_unused) = make_style_image(id, image, options)?;
            command(map, completion, true, move |entry| {
                {
                    let mut kept = entry.observer.kept_style_images.borrow_mut();
                    if keep_unused {
                        kept.insert(image.id.clone());
                    } else {
                        kept.remove(&image.id);
                    }
                }
                with_style(entry, |style| style.add_image(Arc::new(image)));
                Ok(Value::None)
            })
        })
    }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_remove_style_image(
    map: mln_map,
    image_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(image_id, "image_id")?;
            command(map, completion, true, move |entry| {
                with_style(entry, |style| {
                    if style.get_image(&id).is_none() {
                        return Err(Failure::new(MLN_STATUS_NOT_FOUND, format!("no style image has ID {id}")));
                    }
                    style.remove_image(&id);
                    Ok(())
                })?;
                entry.observer.kept_style_images.borrow_mut().remove(&id);
                Ok(Value::None)
            })
        })
    }
}

fn c_stretches(stretches: &ImageStretches) -> Vec<mln_image_stretch> {
    stretches.iter().map(|&(from, to)| mln_image_stretch { from, to }).collect()
}

fn pointer_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() { std::ptr::null() } else { values.as_ptr() }
}

/// # Safety
/// C API contract (style.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_map_get_style_image(
    map: mln_map,
    image_id: mln_buffer_view,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let id = non_empty_text(image_id, "image_id")?;
            command(map, completion, false, move |entry| {
                let Some(image) = entry.map.borrow().get_style().get_image(&id) else {
                    return Ok(Value::None);
                };
                let keep_unused = entry.observer.kept_style_images.borrow().contains(&id);
                // The value shares the style's immutable image, so later style
                // changes leave it whole.
                let stretch_x = c_stretches(&image.stretch_x);
                let stretch_y = c_stretches(&image.stretch_y);
                let pixels = &image.image;
                let content = image.content.unwrap_or_default();
                let mut fields = 0;
                if image.content.is_some() {
                    fields |= MLN_STYLE_IMAGE_INFO_CONTENT;
                }
                if image.text_fit_width.is_some() {
                    fields |= MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH;
                }
                if image.text_fit_height.is_some() {
                    fields |= MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT;
                }
                let info = mln_style_image_info {
                    width: pixels.size.width,
                    height: pixels.size.height,
                    fields,
                    pixels: mln_buffer_view::of(&pixels.data[..pixels.bytes()]),
                    stretch_x: pointer_or_null(&stretch_x),
                    stretch_x_count: stretch_x.len(),
                    stretch_y: pointer_or_null(&stretch_y),
                    stretch_y_count: stretch_y.len(),
                    content: mln_image_content {
                        left: content.left,
                        top: content.top,
                        right: content.right,
                        bottom: content.bottom,
                    },
                    text_fit_width: c_text_fit(image.text_fit_width),
                    text_fit_height: c_text_fit(image.text_fit_height),
                    pixel_ratio: image.pixel_ratio,
                    sdf: image.sdf,
                    keep_unused,
                };
                // SAFETY: the views point into the shared image's pixels and
                // into the stretch vectors' heap buffers, which the storage
                // owns.
                Ok(Value::Lent(Lent::new(vec![info], (image, stretch_x, stretch_y))))
            })
        })
    }
}
