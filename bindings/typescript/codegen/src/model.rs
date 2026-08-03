//! The shape the generator extracts from the public headers.
//!
//! Everything here is ABI description rather than C syntax: the emitters turn it
//! into a C dispatch table, TypeScript layout tables, and the schema fingerprint.

/// One 8-byte argument or return slot in the normalized call ABI.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum SlotKind {
    Void,
    Bool,
    I32,
    U32,
    I64,
    U64,
    F32,
    F64,
    /// `size_t`, whose width follows the ABI class.
    Usize,
    /// Any pointer. The slot carries an address.
    Ptr,
    /// A `uint64_t` handle id.
    Handle,
    /// A struct passed or returned by value. The slot carries the address of a
    /// copy the caller owns.
    Struct,
}

impl SlotKind {
    pub fn as_str(self) -> &'static str {
        match self {
            SlotKind::Void => "void",
            SlotKind::Bool => "bool",
            SlotKind::I32 => "i32",
            SlotKind::U32 => "u32",
            SlotKind::I64 => "i64",
            SlotKind::U64 => "u64",
            SlotKind::F32 => "f32",
            SlotKind::F64 => "f64",
            SlotKind::Usize => "usize",
            SlotKind::Ptr => "ptr",
            SlotKind::Handle => "handle",
            SlotKind::Struct => "struct",
        }
    }
}

#[derive(Clone, Debug)]
pub struct Param {
    pub name: String,
    /// The type as written, used to spell the call in generated C.
    pub spelling: String,
    pub slot: SlotKind,
}

#[derive(Clone, Debug)]
pub struct Entrypoint {
    pub name: String,
    pub header: String,
    pub params: Vec<Param>,
    /// The return type as written.
    pub result_spelling: String,
    pub result: SlotKind,
}

#[derive(Clone, Debug)]
pub struct Field {
    pub name: String,
    pub spelling: String,
    pub offsets: AbiPair,
    pub sizes: AbiPair,
}

/// A value measured once per ABI class.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct AbiPair {
    pub native64: u64,
    pub wasm32: u64,
}

#[derive(Clone, Debug)]
pub struct Record {
    pub name: String,
    pub is_union: bool,
    pub sizes: AbiPair,
    pub aligns: AbiPair,
    pub fields: Vec<Field>,
}

#[derive(Clone, Debug)]
pub struct EnumType {
    pub name: String,
    /// True when the enum's underlying type is signed.
    pub signed: bool,
    pub members: Vec<(String, i64)>,
}

#[derive(Clone, Debug)]
pub struct Constant {
    pub name: String,
    pub value: String,
}

/// A public function whose address a host may store in a struct field.
#[derive(Clone, Debug)]
pub struct Symbol {
    pub name: String,
}

#[derive(Clone, Debug, Default)]
pub struct Api {
    pub entrypoints: Vec<Entrypoint>,
    pub records: Vec<Record>,
    pub enums: Vec<EnumType>,
    pub constants: Vec<Constant>,
    pub symbols: Vec<Symbol>,
}
