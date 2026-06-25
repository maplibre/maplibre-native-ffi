package maplibre

import "testing"

func TestProjectedMetersHelpersRoundTrip(t *testing.T) {
	coordinate := LatLng{Latitude: 45, Longitude: -122}
	meters, err := ProjectedMetersForLatLng(coordinate)
	if err != nil {
		t.Fatalf("ProjectedMetersForLatLng(): %v", err)
	}
	roundTripped, err := LatLngForProjectedMeters(meters)
	if err != nil {
		t.Fatalf("LatLngForProjectedMeters(): %v", err)
	}
	if diff := roundTripped.Latitude - coordinate.Latitude; diff < -1e-9 || diff > 1e-9 {
		t.Fatalf("latitude round trip = %f, want %f", roundTripped.Latitude, coordinate.Latitude)
	}
	if diff := roundTripped.Longitude - coordinate.Longitude; diff < -1e-9 || diff > 1e-9 {
		t.Fatalf("longitude round trip = %f, want %f", roundTripped.Longitude, coordinate.Longitude)
	}
}
