// Snippets for the docs site's Network page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"log"
	"strings"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func withAPIKey(url string) string {
	if strings.Contains(url, "?") {
		return url + "&key=my-api-key"
	}
	return url + "?key=my-api-key"
}

func findTile(url string) []byte { return nil }

func geojsonForTile(tile maplibre.CanonicalTileId) string {
	return `{"type": "FeatureCollection", "features": []}`
}

func network(runtime *maplibre.RuntimeHandle, m *maplibre.MapHandle) error {
	// #region headers
	err := runtime.Discarding().SetHttpHeaderTransform(maplibre.HttpHeaderTransform{
		Rules: []maplibre.HttpHeaderRule{{
			Flags:   maplibre.UrlMatchFlagGlob,
			Url:     "https://tiles.example.com/**",
			Headers: []maplibre.HttpHeader{{Name: "Authorization", Value: "Bearer my-token"}},
		}},
	})
	if err != nil {
		return err
	}
	// #endregion headers

	// #region rewrite
	err = runtime.Discarding().SetResourceTransform(maplibre.ResourceTransform{
		Callback: func(_ maplibre.ResourceKind, url string, request *maplibre.ResourceTransformRequestHandle) {
			if err := request.Complete(withAPIKey(url)); err != nil {
				// Closing the request without an answer keeps the old URL.
				request.Close()
			}
		},
	})
	if err != nil {
		return err
	}
	// #endregion rewrite

	// #region provider
	err = runtime.Discarding().SetResourceProvider(maplibre.ResourceProvider{
		Routes: []maplibre.ResourceRoute{{
			Kinds: maplibre.ResourceKindFlagTile,
			Flags: maplibre.UrlMatchFlagGlob,
			Url:   "app://tiles/**",
		}},
		Callback: func(request maplibre.ResourceRequestInfo, handle *maplibre.ResourceRequestHandle) {
			response := maplibre.ResourceResponse{Status: maplibre.ResourceResponseStatusNoContent}
			if tile := findTile(request.RequestedUrl); tile != nil {
				response = maplibre.ResourceResponse{Status: maplibre.ResourceResponseStatusOk, Bytes: tile}
			}

			if err := handle.Complete(response); err != nil {
				handle.Close()
			}
		},
	})
	if err != nil {
		return err
	}
	// #endregion provider

	// #region custom-source
	err = m.Discarding().AddCustomGeometrySource("app-data", maplibre.CustomGeometrySourceOptions{
		FetchTile: func(tile maplibre.CanonicalTileId) {
			err := m.Discarding().SetCustomGeometrySourceTileData("app-data", tile, geojsonForTile(tile))
			if err != nil {
				log.Print(err)
			}
		},
	})
	if err != nil {
		return err
	}
	// #endregion custom-source

	// #region status
	if err := maplibre.SetNetworkStatus(maplibre.NetworkStatusOffline); err != nil {
		return err
	}

	// Later, when the connection returns:
	if err := maplibre.SetNetworkStatus(maplibre.NetworkStatusOnline); err != nil {
		return err
	}
	// #endregion status
	return nil
}

// An example without an output comment compiles but never runs.
func Example_network() {
	_ = network(nil, nil)
}
