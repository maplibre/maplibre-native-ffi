use std::os::raw::c_char;
use std::ptr;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::{Error, Result};

pub const MAX_JSON_DESCRIPTOR_DEPTH: usize = 64;

/// Ordered JSON object member. Duplicate keys are preserved.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct JsonMember {
    pub key: String,
    pub value: JsonValue,
}

impl JsonMember {
    pub fn new(key: impl Into<String>, value: JsonValue) -> Self {
        Self {
            key: key.into(),
            value,
        }
    }
}

/// Owned JSON-like value tree used by style, GeoJSON, and copied native values.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum JsonValue {
    Null,
    Bool(bool),
    UInt(u64),
    Int(i64),
    Double(f64),
    String(String),
    Array(Vec<JsonValue>),
    Object(Vec<JsonMember>),
}

impl JsonValue {
    pub fn object(members: Vec<JsonMember>) -> Self {
        Self::Object(members)
    }

    pub fn array(values: Vec<JsonValue>) -> Self {
        Self::Array(values)
    }

    pub(crate) fn try_to_native(&self) -> Result<NativeJsonValue> {
        NativeJsonValue::new(self, 0)
    }

    /// Copies a borrowed native JSON value into an owned Rust tree.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this
    /// call. The returned value owns all copied data.
    pub(crate) unsafe fn from_native(raw: &sys::mln_json_value) -> Result<Self> {
        // SAFETY: The caller promises raw and its nested storage are valid for
        // this call. The helper copies recursively before returning.
        unsafe { Self::from_native_with_depth(raw, 0) }
    }

    unsafe fn from_native_with_depth(raw: &sys::mln_json_value, depth: usize) -> Result<Self> {
        check_json_depth(depth)?;
        match raw.type_ {
            sys::MLN_JSON_VALUE_TYPE_NULL => Ok(Self::Null),
            // SAFETY: The active union member is selected by raw.type_.
            sys::MLN_JSON_VALUE_TYPE_BOOL => Ok(Self::Bool(unsafe { raw.data.bool_value })),
            // SAFETY: The active union member is selected by raw.type_.
            sys::MLN_JSON_VALUE_TYPE_UINT => Ok(Self::UInt(unsafe { raw.data.uint_value })),
            // SAFETY: The active union member is selected by raw.type_.
            sys::MLN_JSON_VALUE_TYPE_INT => Ok(Self::Int(unsafe { raw.data.int_value })),
            // SAFETY: The active union member is selected by raw.type_.
            sys::MLN_JSON_VALUE_TYPE_DOUBLE => Ok(Self::Double(unsafe { raw.data.double_value })),
            sys::MLN_JSON_VALUE_TYPE_STRING => {
                // SAFETY: The active union member is selected by raw.type_.
                let view = unsafe { raw.data.string_value };
                // SAFETY: The caller promised nested native string storage is valid.
                Ok(Self::String(unsafe {
                    support::string::copy_string_view(view)
                }?))
            }
            sys::MLN_JSON_VALUE_TYPE_ARRAY => {
                // SAFETY: The active union member is selected by raw.type_.
                let array = unsafe { raw.data.array_value };
                let values = json_values_slice(array.values, array.value_count, "JSON array")?;
                let mut copied = Vec::with_capacity(values.len());
                for value in values {
                    // SAFETY: values came from validated native array storage.
                    copied.push(unsafe { Self::from_native_with_depth(value, depth + 1) }?);
                }
                Ok(Self::Array(copied))
            }
            sys::MLN_JSON_VALUE_TYPE_OBJECT => {
                // SAFETY: The active union member is selected by raw.type_.
                let object = unsafe { raw.data.object_value };
                // SAFETY: The caller promised nested object storage is valid.
                Ok(Self::Object(unsafe {
                    copy_json_members(object.members, object.member_count, depth + 1)
                }?))
            }
            type_ => Err(Error::invalid_argument(format!(
                "unknown native JSON value type: {type_}"
            ))),
        }
    }
}

pub(crate) struct NativeJsonValue {
    raw: sys::mln_json_value,
    strings: Vec<Box<[u8]>>,
    values: Vec<Box<[sys::mln_json_value]>>,
    members: Vec<Box<[sys::mln_json_member]>>,
    children: Vec<NativeJsonValue>,
}

impl NativeJsonValue {
    fn new(value: &JsonValue, depth: usize) -> Result<Self> {
        check_json_depth(depth)?;
        let mut native = Self::empty(sys::MLN_JSON_VALUE_TYPE_NULL);
        match value {
            JsonValue::Null => {}
            JsonValue::Bool(value) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_BOOL;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 { bool_value: *value };
            }
            JsonValue::UInt(value) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_UINT;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 { uint_value: *value };
            }
            JsonValue::Int(value) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_INT;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 { int_value: *value };
            }
            JsonValue::Double(value) => {
                if !value.is_finite() {
                    return Err(Error::invalid_argument("JSON double values must be finite"));
                }
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_DOUBLE;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 {
                    double_value: *value,
                };
            }
            JsonValue::String(value) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_STRING;
                let view = native.store_string(value);
                native.raw.data = sys::mln_json_value__bindgen_ty_1 { string_value: view };
            }
            JsonValue::Array(values) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_ARRAY;
                let raw_values = native.store_child_values(values, depth + 1)?;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 {
                    array_value: sys::mln_json_array {
                        values: ptr_or_null(raw_values.as_ref()),
                        value_count: raw_values.len(),
                    },
                };
                native.values.push(raw_values);
            }
            JsonValue::Object(members) => {
                native.raw.type_ = sys::MLN_JSON_VALUE_TYPE_OBJECT;
                let native_members = native.store_members(members, depth + 1)?;
                native.raw.data = sys::mln_json_value__bindgen_ty_1 {
                    object_value: sys::mln_json_object {
                        members: ptr_or_null(native_members.as_ref()),
                        member_count: native_members.len(),
                    },
                };
                native.members.push(native_members);
            }
        }
        Ok(native)
    }

    fn empty(type_: u32) -> Self {
        Self {
            raw: sys::mln_json_value {
                size: std::mem::size_of::<sys::mln_json_value>() as u32,
                type_,
                data: sys::mln_json_value__bindgen_ty_1 { uint_value: 0 },
            },
            strings: Vec::new(),
            values: Vec::new(),
            members: Vec::new(),
            children: Vec::new(),
        }
    }

    pub(crate) fn as_ptr(&self) -> *const sys::mln_json_value {
        &self.raw
    }

    pub(crate) fn as_ref(&self) -> &sys::mln_json_value {
        &self.raw
    }

    fn store_string(&mut self, value: &str) -> sys::mln_string_view {
        let bytes = value.as_bytes().to_vec().into_boxed_slice();
        let view = sys::mln_string_view {
            data: if bytes.is_empty() {
                ptr::null()
            } else {
                bytes.as_ptr().cast::<c_char>()
            },
            size: bytes.len(),
        };
        self.strings.push(bytes);
        view
    }

    fn store_child_values(
        &mut self,
        values: &[JsonValue],
        depth: usize,
    ) -> Result<Box<[sys::mln_json_value]>> {
        let start = self.children.len();
        for value in values {
            self.children.push(Self::new(value, depth)?);
        }
        Ok(self.children[start..]
            .iter()
            .map(|child| child.raw)
            .collect::<Vec<_>>()
            .into_boxed_slice())
    }

    fn store_members(
        &mut self,
        members: &[JsonMember],
        depth: usize,
    ) -> Result<Box<[sys::mln_json_member]>> {
        let raw_values = self.store_child_values(
            &members
                .iter()
                .map(|member| member.value.clone())
                .collect::<Vec<_>>(),
            depth,
        )?;
        let value_ptr = raw_values.as_ptr();
        let mut native_members = Vec::with_capacity(members.len());
        for (index, member) in members.iter().enumerate() {
            let key = self.store_string(&member.key);
            native_members.push(sys::mln_json_member {
                key,
                // SAFETY: index is within raw_values and raw_values is retained
                // by self.values below for the lifetime of this materializer.
                value: unsafe { value_ptr.add(index) },
            });
        }
        self.values.push(raw_values);
        Ok(native_members.into_boxed_slice())
    }
}

pub(crate) struct NativeJsonMembers {
    raw_members: Box<[sys::mln_json_member]>,
    _raw_values: Box<[sys::mln_json_value]>,
    _strings: Vec<Box<[u8]>>,
    _children: Vec<NativeJsonValue>,
}

impl NativeJsonMembers {
    pub(crate) fn new(members: &[JsonMember], depth: usize) -> Result<Self> {
        check_json_depth(depth)?;
        let mut strings = Vec::with_capacity(members.len());
        let mut children = Vec::with_capacity(members.len());
        for member in members {
            children.push(NativeJsonValue::new(&member.value, depth)?);
        }
        let raw_values = children
            .iter()
            .map(|child| child.raw)
            .collect::<Vec<_>>()
            .into_boxed_slice();
        let value_ptr = raw_values.as_ptr();
        let mut raw_members = Vec::with_capacity(members.len());
        for (index, member) in members.iter().enumerate() {
            let bytes = member.key.as_bytes().to_vec().into_boxed_slice();
            let key = sys::mln_string_view {
                data: if bytes.is_empty() {
                    ptr::null()
                } else {
                    bytes.as_ptr().cast::<c_char>()
                },
                size: bytes.len(),
            };
            strings.push(bytes);
            raw_members.push(sys::mln_json_member {
                key,
                // SAFETY: index is within raw_values, which this struct retains.
                value: unsafe { value_ptr.add(index) },
            });
        }
        Ok(Self {
            raw_members: raw_members.into_boxed_slice(),
            _raw_values: raw_values,
            _strings: strings,
            _children: children,
        })
    }

    pub(crate) fn as_ptr(&self) -> *const sys::mln_json_member {
        ptr_or_null(self.raw_members.as_ref())
    }

    pub(crate) fn len(&self) -> usize {
        self.raw_members.len()
    }
}

fn check_json_depth(depth: usize) -> Result<()> {
    if depth > MAX_JSON_DESCRIPTOR_DEPTH {
        Err(Error::invalid_argument(format!(
            "JSON descriptor depth exceeds {MAX_JSON_DESCRIPTOR_DEPTH}"
        )))
    } else {
        Ok(())
    }
}

pub(crate) unsafe fn copy_json_members(
    members: *const sys::mln_json_member,
    count: usize,
    depth: usize,
) -> Result<Vec<JsonMember>> {
    check_json_depth(depth)?;
    let members = json_members_slice(members, count, "JSON object members")?;
    let mut copied = Vec::with_capacity(members.len());
    for member in members {
        // SAFETY: The caller promised member key and value storage is valid.
        let key = unsafe { support::string::copy_string_view(member.key) }?;
        if member.value.is_null() {
            return Err(Error::invalid_argument(
                "JSON object member value must not be null",
            ));
        }
        // SAFETY: member.value was checked non-null and storage validity is
        // promised by the caller.
        let value = unsafe { JsonValue::from_native_with_depth(&*member.value, depth) }?;
        copied.push(JsonMember { key, value });
    }
    Ok(copied)
}

fn json_values_slice<'a>(
    ptr: *const sys::mln_json_value,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_json_value]> {
    if count == 0 {
        return Ok(&[]);
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(format!(
            "{name} pointer must not be null when count is nonzero"
        )));
    }
    // SAFETY: The caller promises ptr points to count valid JSON values.
    Ok(unsafe { std::slice::from_raw_parts(ptr, count) })
}

fn json_members_slice<'a>(
    ptr: *const sys::mln_json_member,
    count: usize,
    name: &'static str,
) -> Result<&'a [sys::mln_json_member]> {
    if count == 0 {
        return Ok(&[]);
    }
    if ptr.is_null() {
        return Err(Error::invalid_argument(format!(
            "{name} pointer must not be null when count is nonzero"
        )));
    }
    // SAFETY: The caller promises ptr points to count valid JSON members.
    Ok(unsafe { std::slice::from_raw_parts(ptr, count) })
}

fn ptr_or_null<T>(values: &[T]) -> *const T {
    if values.is_empty() {
        ptr::null()
    } else {
        values.as_ptr()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn json_values_materialize_and_copy_back_preserving_order_duplicates_and_width() {
        let value = JsonValue::object(vec![
            JsonMember::new("null", JsonValue::Null),
            JsonMember::new("bool", JsonValue::Bool(true)),
            JsonMember::new("uint", JsonValue::UInt(u64::MAX)),
            JsonMember::new("int", JsonValue::Int(-7)),
            JsonMember::new("double", JsonValue::Double(1.25)),
            JsonMember::new("string", JsonValue::String("hello\0world".to_owned())),
            JsonMember::new("dup", JsonValue::Int(1)),
            JsonMember::new("dup", JsonValue::Int(2)),
            JsonMember::new(
                "array",
                JsonValue::Array(vec![
                    JsonValue::String("a".to_owned()),
                    JsonValue::String("b".to_owned()),
                ]),
            ),
        ]);

        let native = value.try_to_native().unwrap();
        assert_eq!(native.as_ref().type_, sys::MLN_JSON_VALUE_TYPE_OBJECT);
        // SAFETY: native owns a valid JSON descriptor graph for this test.
        let copied = unsafe { JsonValue::from_native(native.as_ref()) }.unwrap();

        assert_eq!(copied, value);
    }

    #[test]
    fn json_depth_limit_rejects_too_deep_descriptors_and_cleans_up_allocations() {
        let mut value = JsonValue::Null;
        for _ in 0..=MAX_JSON_DESCRIPTOR_DEPTH + 1 {
            value = JsonValue::Array(vec![value]);
        }

        let error = value.try_to_native().err().unwrap();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("JSON descriptor depth exceeds"));
    }

    #[test]
    fn json_rejects_non_finite_numbers_before_calling_c() {
        let error = JsonValue::Double(f64::NAN).try_to_native().err().unwrap();

        assert_eq!(error.kind(), crate::ErrorKind::InvalidArgument);
        assert!(error.diagnostic().contains("finite"));
    }
}
