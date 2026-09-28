/// Value conversion used by generated plain records.
#[doc(hidden)]
pub trait NativeValue: Sized {
    type Raw;
    fn to_native(self) -> Self::Raw;
    fn from_native(value: Self::Raw) -> Self;
}
