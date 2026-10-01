//! Calls the bindings that the probe generated from the fixture headers,
//! through the binding's handwritten runtime, against protocols_stub.c.

use crate::*;

pub fn run() {
    // Fields named after Rust keywords and runtime locals keep their values
    // through both conversions.
    let point = NewPoint::new(1.0, 2.0, 3.0);
    let raw: maplibre_native_ffi_sys::mln_new_point =
        ToNative::to_native(&point, &mut InputArena::default()).unwrap();
    assert_eq!((raw.type_, raw.self_, raw.str_), (1.0, 2.0, 3.0));
    // SAFETY: the record holds no pointers.
    assert_eq!(unsafe { NewPoint::from_native(raw) }.unwrap(), point);

    // Parameters named after keywords and runtime locals keep their order.
    let entry = keyword_combine(5.0, 2.0, 7.0, 11.0).unwrap();
    assert_eq!(entry, KeywordEntry::new(3.0, 7.0, 11.0));

    // An owner releases its handle once.
    let map = MapHandle::adopt(maplibre_native_ffi_sys::mln_map(7), None).unwrap();
    assert_eq!(map.id(), 7);
    map.release().unwrap();
    assert!(map.is_closed());
    map.release().unwrap();
}
