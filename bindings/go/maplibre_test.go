package maplibre

import "context"

func awaitForTest[T any](future *Future[T], err error) (T, error) {
	var zero T
	if err != nil {
		return zero, err
	}
	return future.Await(context.Background())
}

// closeRuntimeForTest closes a runtime and waits for its native teardown, so a
// test leaves no native thread running past its own end.
func closeRuntimeForTest(runtime *RuntimeHandle) error {
	_, err := awaitForTest(runtime.Close())
	return err
}

const minimalStyleJSON = `{
  "version": 8,
  "name": "go-binding-style-test",
  "sources": {},
  "layers": [
    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
  ]
}`
