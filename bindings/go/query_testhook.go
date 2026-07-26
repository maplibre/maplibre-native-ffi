package maplibre

/*
#include "maplibre_native_c.h"

static inline void mln_go_accept_rendered_query_geometry(
  const mln_rendered_query_geometry* geometry
) {
  (void)geometry;
}

static inline void mln_go_accept_rendered_feature_query_options(
  const mln_rendered_feature_query_options* options
) {
  (void)options;
}

static inline void mln_go_accept_source_feature_query_options(
  const mln_source_feature_query_options* options
) {
  (void)options;
}
*/
import "C"

func renderedQueryGeometryPassesCgoPointerCheckForTest(geometry RenderedQueryGeometry) {
	raw := newCRenderedQueryGeometry(geometry)
	defer raw.free()
	C.mln_go_accept_rendered_query_geometry(raw.ptr())
}

func renderedFeatureQueryOptionsPassesCgoPointerCheckForTest(options *RenderedFeatureQueryOptions) error {
	raw, err := newCRenderedFeatureQueryOptions(options)
	if err != nil {
		return err
	}
	defer raw.free()
	C.mln_go_accept_rendered_feature_query_options(raw.ptr())
	return nil
}

func sourceFeatureQueryOptionsPassesCgoPointerCheckForTest(options *SourceFeatureQueryOptions) error {
	raw, err := newCSourceFeatureQueryOptions(options)
	if err != nil {
		return err
	}
	defer raw.free()
	C.mln_go_accept_source_feature_query_options(raw.ptr())
	return nil
}
