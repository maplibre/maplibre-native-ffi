package maplibre

import (
	stdruntime "runtime"
	"testing"
)

const minimalStyleJSON = `{
  "version": 8,
  "name": "go-binding-style-test",
  "sources": {},
  "layers": [
    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
  ]
}`

func lockOSThreadForTest(t *testing.T) {
	t.Helper()
	stdruntime.LockOSThread()
	t.Cleanup(stdruntime.UnlockOSThread)
}
