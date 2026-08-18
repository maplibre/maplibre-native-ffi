use std::ptr;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::{NativeFuture, Result};

pub use maplibre_core::query::{
    FeatureStateSelector, QueriedFeature, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
pub(crate) use maplibre_core::query::{
    NativeRenderedFeatureQueryOptions, NativeSourceFeatureQueryOptions,
    RenderedFeatureQueryOptionsNativeExt, RenderedQueryGeometryNativeExt,
    SourceFeatureQueryOptionsNativeExt,
};

impl super::RenderSessionHandle {
    fn selector_views(
        selector: &FeatureStateSelector,
    ) -> (
        sys::mln_buffer_view,
        sys::mln_buffer_view,
        sys::mln_buffer_view,
        sys::mln_buffer_view,
    ) {
        (
            maplibre_core::string::buffer_view(selector.source_id().as_bytes()),
            maplibre_core::string::buffer_view(
                selector.source_layer_id().unwrap_or_default().as_bytes(),
            ),
            maplibre_core::string::buffer_view(
                selector.feature_id().unwrap_or_default().as_bytes(),
            ),
            maplibre_core::string::buffer_view(selector.state_key().unwrap_or_default().as_bytes()),
        )
    }

    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector,
        state: &[u8],
    ) -> Result<NativeFuture<()>> {
        let (source, layer, feature, _) = Self::selector_views(selector);
        let state = maplibre_core::string::buffer_view(state);
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_set_feature_state(
                    session, source, layer, feature, state, completion,
                )
            },
            crate::completion::unit,
        )
    }

    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<Vec<u8>>> {
        let (source, layer, feature, _) = Self::selector_views(selector);
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_get_feature_state(
                    session, source, layer, feature, completion,
                )
            },
            crate::completion::buffer,
        )
    }

    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<NativeFuture<()>> {
        let (source, layer, feature, key) = Self::selector_views(selector);
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_remove_feature_state(
                    session, source, layer, feature, key, completion,
                )
            },
            crate::completion::unit,
        )
    }

    /// Starts a rendered-feature query whose result is a typed feature list.
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<QueriedFeature>>> {
        let geometry = geometry.to_native();
        let options = options
            .map(RenderedFeatureQueryOptions::to_native)
            .transpose()?;
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_query_rendered_features(
                    session,
                    geometry.as_ptr(),
                    options
                        .as_ref()
                        .map_or(ptr::null(), NativeRenderedFeatureQueryOptions::as_ptr),
                    completion,
                )
            },
            copy_queried_features,
        )
    }

    /// Starts a source-feature query whose result is a typed feature list.
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<QueriedFeature>>> {
        let source_id = maplibre_core::string::buffer_view(source_id.as_bytes());
        let options = options
            .map(SourceFeatureQueryOptions::to_native)
            .transpose()?;
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_query_source_features(
                    session,
                    source_id,
                    options
                        .as_ref()
                        .map_or(ptr::null(), NativeSourceFeatureQueryOptions::as_ptr),
                    completion,
                )
            },
            copy_queried_features,
        )
    }

    /// Starts a feature-extension query whose result is UTF-8 JSON or GeoJSON
    /// bytes.
    pub fn query_feature_extension(
        &self,
        source_id: &str,
        feature: &[u8],
        extension: &str,
        extension_field: &str,
        arguments: Option<&[u8]>,
    ) -> Result<NativeFuture<Vec<u8>>> {
        let source_id = maplibre_core::string::buffer_view(source_id.as_bytes());
        let feature = maplibre_core::string::buffer_view(feature);
        let extension = maplibre_core::string::buffer_view(extension.as_bytes());
        let extension_field = maplibre_core::string::buffer_view(extension_field.as_bytes());
        let arguments = arguments.map(maplibre_core::string::buffer_view);
        let session = self.inner.native()?;
        crate::completion::submit(
            |completion| unsafe {
                sys::mln_render_session_query_feature_extensions(
                    session,
                    source_id,
                    feature,
                    extension,
                    extension_field,
                    arguments.as_ref().map_or(ptr::null(), ptr::from_ref),
                    completion,
                )
            },
            crate::completion::buffer,
        )
    }
}

fn copy_queried_features(result: &sys::mln_completion_result) -> Result<Vec<QueriedFeature>> {
    let hits = crate::completion::copy_slice::<sys::mln_queried_feature>(result)?;
    // SAFETY: completion keeps every nested feature view alive for this callback.
    unsafe { maplibre_core::query::copy_queried_features(&hits) }
}
