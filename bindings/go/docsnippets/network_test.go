// Snippets for the docs site's Network page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"log"
	"strings"

	"maplibre.org/maplibre-native-ffi/go/maplibre"
)

func withAPIKey(url string) string {
	if strings.Contains(url, "?") {
		return url + "&key=my-api-key"
	}
	return url + "?key=my-api-key"
}

func findTile(url string) []byte { return nil }

func geojsonForTile(tile maplibre.CanonicalTileID) string {
	return `{"type": "FeatureCollection", "features": []}`
}

func network(runtime *maplibre.RuntimeHandle, m *maplibre.MapHandle) error {
	// #region headers
	headers := maplibre.HTTPHeaderTransform{
		Rules: []maplibre.HTTPHeaderRule{{
			Flags: maplibre.URLMatchFlagGlob,
			URL:   "https://tiles.example.com/**",
			Headers: []maplibre.HTTPHeader{{
				Name:  "Authorization",
				Value: "Bearer my-token",
			}},
		}},
	}
	err := runtime.Discarding().SetHTTPHeaderTransform(headers)
	if err != nil {
		return err
	}
	// #endregion headers

	// #region query-rules
	keys := maplibre.ResourceTransform{
		Rules: []maplibre.ResourceTransformRule{{
			Flags: maplibre.URLMatchFlagGlob,
			URL:   "https://tiles.example.com/**",
			Parameters: []maplibre.URLQueryParameter{{
				Name:  "key",
				Value: "my-api-key",
			}},
		}},
	}
	err = runtime.Discarding().SetResourceTransform(keys)
	if err != nil {
		return err
	}
	// #endregion query-rules

	// #region rewrite
	rewrite := maplibre.ResourceTransform{
		Callback: func(
			_ maplibre.ResourceKind,
			url string,
			request *maplibre.ResourceTransformRequestHandle,
		) {
			// The answer ends the request whatever it returns. A failed
			// one fails the request.
			if err := request.Complete(withAPIKey(url)); err != nil {
				log.Print(err)
			}
		},
	}
	err = runtime.Discarding().SetResourceTransform(rewrite)
	if err != nil {
		return err
	}
	// #endregion rewrite

	// #region provider
	provider := maplibre.ResourceProvider{
		Routes: []maplibre.ResourceRoute{{
			Kinds: maplibre.ResourceKindFlagTile,
			Flags: maplibre.URLMatchFlagGlob,
			URL:   "app://tiles/**",
		}},
		Callback: func(
			request maplibre.ResourceRequestInfo,
			handle *maplibre.ResourceRequestHandle,
		) {
			response := maplibre.ResourceResponse{
				Status: maplibre.ResourceResponseStatusNoContent,
			}
			if tile := findTile(request.RequestedURL); tile != nil {
				response = maplibre.ResourceResponse{
					Status: maplibre.ResourceResponseStatusOk,
					Bytes:  tile,
				}
			}

			if err := handle.Complete(response); err != nil {
				log.Print(err)
			}
		},
	}
	err = runtime.Discarding().SetResourceProvider(provider)
	if err != nil {
		return err
	}
	// #endregion provider

	// #region custom-source
	source := maplibre.CustomGeometrySourceOptions{
		FetchTile: func(
			m *maplibre.MapHandle, sourceID string, tile maplibre.CanonicalTileID,
		) {
			err := m.Discarding().SetCustomGeometrySourceTileData(
				sourceID, tile, geojsonForTile(tile),
			)
			if err != nil {
				log.Print(err)
			}
		},
	}
	err = m.Discarding().AddCustomGeometrySource("app-data", source)
	if err != nil {
		return err
	}
	// #endregion custom-source

	// #region status
	err = maplibre.SetNetworkStatus(maplibre.NetworkStatusOffline)
	if err != nil {
		return err
	}

	// Later, when the connection returns:
	err = maplibre.SetNetworkStatus(maplibre.NetworkStatusOnline)
	if err != nil {
		return err
	}
	// #endregion status
	return nil
}

// An example without an output comment compiles but never runs.
func Example_network() {
	_ = network(nil, nil)
}
