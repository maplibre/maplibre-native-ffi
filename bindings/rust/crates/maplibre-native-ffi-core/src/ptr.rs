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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn strided_records_are_read_at_their_stride_and_a_narrower_one_fails() {
        // Two records, each followed by a member this binding does not know.
        #[repr(C)]
        #[derive(Clone, Copy)]
        struct Wide {
            record: [f64; 2],
            newer: f64,
        }
        let wide = [
            Wide {
                record: [1.0, 2.0],
                newer: -1.0,
            },
            Wide {
                record: [3.0, 4.0],
                newer: -1.0,
            },
        ];
        let pointer = wide.as_ptr().cast::<[f64; 2]>();
        // SAFETY: wide holds two records at its stride.
        let records = unsafe { strided_records(pointer, 2, std::mem::size_of::<Wide>()) }
            .unwrap()
            .collect::<Vec<_>>();
        assert_eq!(records, [[1.0, 2.0], [3.0, 4.0]]);

        // SAFETY: a stride narrower than the record fails before any read.
        let narrow = unsafe { strided_records(pointer, 2, std::mem::size_of::<[f64; 2]>() - 1) };
        assert_eq!(narrow.err().unwrap().kind(), ErrorKind::NativeError);
    }
}
