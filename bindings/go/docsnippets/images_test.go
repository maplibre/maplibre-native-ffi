// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"maplibre.org/maplibre-native-ffi/bindings/go/maplibre"
)

func copyPixels(data []byte, stride uint32) {}

func images(ctx context.Context, runtime *maplibre.RuntimeHandle) error {
	// #region map
	m, created, err := runtime.CreateMap(maplibre.MapOptions{
		InitialExtent: maplibre.LogicalExtent{
			Width: 800, Height: 600, ScaleFactor: 2,
		},
		MapMode: maplibre.MapModeStatic,
	})
	if err != nil {
		return err
	}
	if _, err := created.Await(ctx); err != nil {
		return err
	}
	// #endregion map

	// #region style
	err = m.Discarding().SetStyleURL(
		"https://demotiles.maplibre.org/style.json",
	)
	if err != nil {
		return err
	}
	// #endregion style

	// #region attach
	session, attached, err := m.AttachOffscreen(
		maplibre.OffscreenDescriptor{
			LogicalSize: maplibre.LogicalSize{Width: 800, Height: 600},
		},
		// The library renders on its own thread.
		maplibre.RenderSessionAttachOptions{
			Driver: maplibre.RenderDriverKindCoreWorker,
		},
		// With no device, the session creates its own.
		nil,
	)
	if err != nil {
		return err
	}
	if _, err := attached.Await(ctx); err != nil {
		return err
	}
	// #endregion attach

	// #region image
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode: maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{
			Center: &maplibre.LatLng{Latitude: 48.8566, Longitude: 2.3522},
			Zoom:   new(4.0),
		},
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

	released, err := m.Release()
	if err != nil {
		return err
	}
	if _, err := released.Await(ctx); err != nil {
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

	camera, err := maplibre.CameraForTile(
		maplibre.CanonicalTileID{Z: z, X: x, Y: y},
		maplibre.LogicalSize{Width: 512, Height: 512},
	)
	if err != nil {
		return err
	}
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode:   maplibre.CameraUpdateModeJump,
		Camera: camera,
	})
	if err != nil {
		return err
	}
	// #endregion tile

	released, err := m.Release()
	if err != nil {
		return err
	}
	_, err = released.Await(ctx)
	return err
}

// An example without an output comment compiles but never runs.
func Example_images() {
	ctx := context.Background()
	_ = images(ctx, nil)
	_ = imagesTile(ctx, nil, 0, 0, 0)
}
