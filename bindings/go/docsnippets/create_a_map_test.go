// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func check(err error) {
	if err != nil {
		panic(err)
	}
}

func copyPixels(data []byte, stride uint32) {}

// An example without an output comment compiles but never runs.
func Example_createAMap() {
	ctx := context.Background()

	// #region runtime
	runtime, err := maplibre.NewRuntime(maplibre.RuntimeOptions{})
	check(err)
	// #endregion runtime

	// #region map
	mapOptions := maplibre.MapOptions{
		InitialExtent: maplibre.LogicalExtent{Width: 512, Height: 512, ScaleFactor: 1},
		MapMode:       maplibre.MapModeStatic,
	}

	m, created, err := runtime.CreateMap(mapOptions)
	check(err)
	_, err = created.Await(ctx)
	check(err)
	// #endregion map

	// #region style
	check(m.Discarding().SetStyleUrl("https://demotiles.maplibre.org/style.json"))
	// #endregion style

	// #region attach
	// nil selects the default adapter.
	device, err := maplibre.NewGraphicsDevice(nil)
	check(err)

	texture := maplibre.GraphicsDeviceTextureDescriptor{Extent: mapOptions.InitialExtent}
	// The library renders on its own thread.
	attachOptions := maplibre.RenderSessionAttachOptions{Driver: maplibre.RenderDriverKindCoreWorker}

	session, attached, err := m.AttachGraphicsDeviceTexture(device, texture, attachOptions)
	check(err)
	_, err = attached.Await(ctx)
	check(err)
	// #endregion attach

	// #region render
	rendered, err := m.RequestStillImage()
	check(err)
	_, err = rendered.Await(ctx)
	check(err)
	// #endregion render

	// #region render
	read, err := session.ReadTexture()
	check(err)
	image, err := read.Await(ctx)
	check(err)
	// #endregion render

	// #region render
	copyPixels(image.Data, image.Info.Stride)
	// #endregion render

	// #region close
	check(session.Dispose())
	check(device.Close())

	mapClosed, err := m.Close()
	check(err)
	_, err = mapClosed.Await(ctx)
	check(err)

	runtimeClosed, err := runtime.Close()
	check(err)
	_, err = runtimeClosed.Await(ctx)
	check(err)
	// #endregion close
}
