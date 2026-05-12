use maplibre_native::{
    NetworkStatus, RenderBackendMask, c_version, network_status, set_network_status,
    supported_render_backends,
};

struct NetworkStatusRestore(NetworkStatus);

impl Drop for NetworkStatusRestore {
    fn drop(&mut self) {
        let _ = set_network_status(self.0);
    }
}

#[test]
fn public_api_uses_safe_rust_types() {
    assert_eq!(c_version(), 0);

    let backends: RenderBackendMask = supported_render_backends();
    assert!(backends.intersects(RenderBackendMask::METAL | RenderBackendMask::VULKAN));

    let original: NetworkStatus = network_status().unwrap();
    let _restore = NetworkStatusRestore(original);

    set_network_status(NetworkStatus::Offline).unwrap();
    assert_eq!(network_status().unwrap(), NetworkStatus::Offline);
}
