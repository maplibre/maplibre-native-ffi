mod bindings {
    #![allow(clippy::all)]
    #![allow(non_camel_case_types)]
    #![allow(non_snake_case)]
    #![allow(non_upper_case_globals)]
    #![allow(unsafe_op_in_unsafe_fn)]

    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub use bindings::*;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reports_c_abi_version() {
        let version = unsafe { mln_c_version() };

        assert_eq!(version, 0);
    }

    #[test]
    fn reports_supported_render_backend_mask() {
        let mask = unsafe { mln_supported_render_backend_mask() };
        let known_backends = MLN_RENDER_BACKEND_FLAG_METAL | MLN_RENDER_BACKEND_FLAG_VULKAN;

        assert_eq!(mask & !known_backends, 0);
    }
}
