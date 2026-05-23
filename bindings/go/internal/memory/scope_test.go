package memory

import "testing"

func TestKeepAliveAllAcceptsNilAndValues(t *testing.T) {
	value := []byte{1, 2, 3}
	KeepAliveAll(nil, value)
}
