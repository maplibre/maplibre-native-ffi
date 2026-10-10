//go:build !unix || !cgo

package testsupport

// WriteCoverageProfile does nothing on a platform without coverage builds.
func WriteCoverageProfile() error {
	return nil
}
