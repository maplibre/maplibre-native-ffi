package maplibre

import (
	"fmt"
	"os"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"
)

func TestMain(m *testing.M) {
	code := m.Run()
	if err := testsupport.WriteCoverageProfile(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		if code == 0 {
			code = 1
		}
	}
	os.Exit(code)
}
