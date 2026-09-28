//! Integration tests that drive the public binding against the native library.

mod map;
mod projection;
mod render;
mod runtime;

use crate::*;

#[test]

fn projected_meter_helpers_round_trip() {
    let coordinate = LatLng::new(45.0, -122.0);
    let meters = projected_meters_for_lat_lng(coordinate).unwrap();
    let round_tripped = lat_lng_for_projected_meters(meters).unwrap();

    assert!((round_tripped.latitude - coordinate.latitude).abs() < 1e-9);
    assert!((round_tripped.longitude - coordinate.longitude).abs() < 1e-9);
}

#[test]

fn unknown_network_status_is_rejected_by_native() {
    let error = network_status_set(NetworkStatus::Unknown(999_999)).unwrap_err();

    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    assert!(error.diagnostic().contains("network status"));
}
