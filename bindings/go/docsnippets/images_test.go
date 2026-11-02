// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
func tileCenter(z, x, y uint32) maplibre.LatLng {
	return maplibre.LatLng{}
}

func copyPixels(data []byte, stride uint32) {}

func images(ctx context.Context, runtime *maplibre.RuntimeHandle) error {
	// #region map
	extent := maplibre.LogicalExtent{
		Width: 800, Height: 600, ScaleFactor: 2,
	}
	m, created, err := runtime.CreateMap(maplibre.MapOptions{
		InitialExtent: extent,
		MapMode:       maplibre.MapModeStatic,
	})
	if err != nil {
		return err
	}
	if _, err := created.Await(ctx); err != nil {
		return err
	}
	// #endregion map

	// #region style
	err = m.Discarding().SetStyleUrl(
		"https://demotiles.maplibre.org/style.json",
	)
	if err != nil {
		return err
	}
	// #endregion style

	// #region attach
	// nil options select the default adapter.
	device, err := maplibre.NewGraphicsDevice(nil)
	if err != nil {
		return err
	}

	session, attached, err := m.AttachGraphicsDeviceTexture(
		device,
		maplibre.GraphicsDeviceTextureDescriptor{Extent: extent},
		// The library renders on its own thread.
		maplibre.RenderSessionAttachOptions{
			Driver: maplibre.RenderDriverKindCoreWorker,
		},
	)
	if err != nil {
		return err
	}
	if _, err := attached.Await(ctx); err != nil {
		return err
	}
	// #endregion attach

	// #region image
	center := maplibre.LatLng{Latitude: 48.8566, Longitude: 2.3522}
	zoom := 4.0
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode:   maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{Center: &center, Zoom: &zoom},
	})
	if err != nil {
		return err
	}

	rendered, err := m.RequestStillImage()
	if err != nil {
		return err
	}
	if _, err := rendered.Await(ctx); err != nil {
		return err
	}
	// #endregion image

	// #region read
	read, err := session.ReadTexture()
	if err != nil {
		return err
	}
	image, err := read.Await(ctx)
	if err != nil {
		return err
	}
	copyPixels(image.Data, image.Info.Stride)
	// #endregion read

	if err := session.Dispose(); err != nil {
		return err
	}
	if err := device.Close(); err != nil {
		return err
	}

	closed, err := m.Close()
	if err != nil {
		return err
	}
	if _, err := closed.Await(ctx); err != nil {
		return err
	}
	return nil
}

func imagesTile(ctx context.Context, runtime *maplibre.RuntimeHandle, z, x, y uint32) error {
	// #region tile
	m, created, err := runtime.CreateMap(maplibre.MapOptions{
		InitialExtent: maplibre.LogicalExtent{
			Width: 512, Height: 512, ScaleFactor: 1,
		},
		MapMode: maplibre.MapModeTile,
	})
	if err != nil {
		return err
	}
	if _, err := created.Await(ctx); err != nil {
		return err
	}

	// A 512-pixel map shows tile z/x/y at zoom z.
	center, zoom := tileCenter(z, x, y), float64(z)
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode:   maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{Center: &center, Zoom: &zoom},
	})
	if err != nil {
		return err
	}
	// #endregion tile

	closed, err := m.Close()
	if err != nil {
		return err
	}
	_, err = closed.Await(ctx)
	return err
}

// An example without an output comment compiles but never runs.
func Example_images() {
	ctx := context.Background()
	_ = images(ctx, nil)
	_ = imagesTile(ctx, nil, 0, 0, 0)
}
