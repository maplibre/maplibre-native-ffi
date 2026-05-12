#![deny(unsafe_op_in_unsafe_fn)]

mod handle;
mod map;
mod runtime;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

pub use map::MapHandle;
pub use runtime::RuntimeHandle;
pub use support::{Error, ErrorKind, Result};

bitflags::bitflags! {
    /// Render backends compiled into the linked native library.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct RenderBackendMask: u32 {
        const METAL = sys::MLN_RENDER_BACKEND_FLAG_METAL;
        const VULKAN = sys::MLN_RENDER_BACKEND_FLAG_VULKAN;
        const _ = !0;
    }
}

/// Process-global network reachability state used by MapLibre Native.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum NetworkStatus {
    Online,
    Offline,
    Unknown(u32),
}

impl NetworkStatus {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_NETWORK_STATUS_ONLINE => Self::Online,
            sys::MLN_NETWORK_STATUS_OFFLINE => Self::Offline,
            _ => Self::Unknown(raw),
        }
    }

    fn raw(self) -> Result<u32> {
        match self {
            Self::Online => Ok(sys::MLN_NETWORK_STATUS_ONLINE),
            Self::Offline => Ok(sys::MLN_NETWORK_STATUS_OFFLINE),
            Self::Unknown(raw) => Err(Error::invalid_argument(format!(
                "unknown network status values cannot be set: {raw}"
            ))),
        }
    }
}

/// Returns the native C ABI contract version.
pub fn c_version() -> u32 {
    // SAFETY: mln_c_version takes no arguments and returns the process-global C
    // ABI version for the linked native library.
    unsafe { sys::mln_c_version() }
}

/// Returns the render backends compiled into the linked native library.
pub fn supported_render_backends() -> RenderBackendMask {
    // SAFETY: mln_supported_render_backend_mask takes no arguments and returns a
    // value mask. Unknown future bits are preserved by from_bits_retain.
    let mask = unsafe { sys::mln_supported_render_backend_mask() };
    RenderBackendMask::from_bits_retain(mask)
}

/// Reads MapLibre Native's process-global network status.
pub fn network_status() -> Result<NetworkStatus> {
    let mut raw_status = 0;
    // SAFETY: out_status points to valid writable storage for one u32.
    support::check(unsafe { sys::mln_network_status_get(&mut raw_status) })?;
    Ok(NetworkStatus::from_raw(raw_status))
}

/// Sets MapLibre Native's process-global network status.
pub fn set_network_status(status: NetworkStatus) -> Result<()> {
    set_network_status_raw(status.raw()?)
}

fn set_network_status_raw(raw_status: u32) -> Result<()> {
    // SAFETY: The raw value is passed by value. The C API validates the enum
    // domain and reports invalid values as MLN_STATUS_INVALID_ARGUMENT.
    support::check(unsafe { sys::mln_network_status_set(raw_status) })
}

#[cfg(test)]
mod tests {
    use static_assertions::assert_not_impl_any;

    use super::*;

    assert_not_impl_any!(RuntimeHandle: Send, Sync);
    assert_not_impl_any!(MapHandle: Send, Sync);

    struct NetworkStatusRestore(NetworkStatus);

    impl Drop for NetworkStatusRestore {
        fn drop(&mut self) {
            let _ = set_network_status(self.0);
        }
    }

    #[test]
    fn reports_c_abi_version() {
        assert_eq!(c_version(), support::EXPECTED_C_ABI_VERSION);
    }

    #[test]
    fn reports_supported_render_backends() {
        let backends = supported_render_backends();
        let known_backends = RenderBackendMask::METAL | RenderBackendMask::VULKAN;

        assert!(backends.intersects(known_backends));
    }

    #[test]
    fn network_status_round_trips() {
        let original = network_status().unwrap();
        let _restore = NetworkStatusRestore(original);

        set_network_status(NetworkStatus::Offline).unwrap();
        assert_eq!(network_status().unwrap(), NetworkStatus::Offline);

        set_network_status(NetworkStatus::Online).unwrap();
        assert_eq!(network_status().unwrap(), NetworkStatus::Online);
    }

    #[test]
    fn invalid_network_status_reports_public_error() {
        let error = set_network_status_raw(999_999).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(error.diagnostic().contains("network status"));
    }

    #[test]
    fn unknown_network_status_output_preserves_raw_value() {
        assert_eq!(
            NetworkStatus::from_raw(999_999),
            NetworkStatus::Unknown(999_999)
        );
    }

    #[test]
    fn unknown_network_status_is_rejected_before_calling_c() {
        let error = set_network_status(NetworkStatus::Unknown(999_999)).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), None);
        assert!(error.diagnostic().contains("cannot be set"));
    }
}
