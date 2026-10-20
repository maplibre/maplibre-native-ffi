use crate::error::{Error, ErrorKind, Result};

/// The error for a handle argument that holds the null handle.
pub fn null_handle_error(name: &'static str) -> Error {
    Error::invalid_argument(format!("{name} must not be the null handle"))
}

/// Reads `count` records that native lays out `stride` bytes apart.
///
/// A native build whose record grew reports a stride wider than this
/// binding's record, so each record is read at its stride and the members this
/// binding does not know are skipped. A stride narrower than the record cannot
/// hold one and fails.
///
/// # Safety
///
/// A nonzero `count` needs a pointer that addresses `count` initialized
/// records at the given stride, which stay valid while the iterator runs.
pub unsafe fn strided_records<T: Copy>(
    pointer: *const T,
    count: usize,
    stride: usize,
) -> Result<impl Iterator<Item = T>> {
    if count != 0
        && (pointer.is_null()
            || stride < std::mem::size_of::<T>()
            || count > isize::MAX as usize / stride)
    {
        return Err(Error::new(
            ErrorKind::NativeError,
            None,
            "invalid native record stride",
        ));
    }
    Ok((0..count).map(move |index| {
        // SAFETY: the caller supplies count records of the given stride.
        unsafe {
            pointer
                .cast::<u8>()
                .add(index * stride)
                .cast::<T>()
                .read_unaligned()
        }
    }))
}
