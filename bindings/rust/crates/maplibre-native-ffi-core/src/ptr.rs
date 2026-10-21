use crate::{Error, Result, Status};

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
            Status::NativeError,
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

/// Borrows an initialized native array for the caller's borrow lifetime.
///
/// # Safety
/// A nonempty pointer must address `count` initialized elements for that lifetime.
pub unsafe fn slice<'a, T>(pointer: *const T, count: usize) -> Result<&'a [T]> {
    if count == 0 {
        return Ok(&[]);
    }
    if pointer.is_null()
        || !pointer.is_aligned()
        || count > isize::MAX as usize / std::mem::size_of::<T>().max(1)
    {
        return Err(Error::invalid_argument("invalid native array"));
    }
    // SAFETY: the caller guarantees count initialized values.
    Ok(unsafe { std::slice::from_raw_parts(pointer, count) })
}
