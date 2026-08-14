use std::ptr;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::Result;

pub use maplibre_core::query::{
    FeatureStateSelector, RenderedFeatureQueryOptions, RenderedQueryGeometry,
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
    ) -> Result<crate::runtime::OperationHandle<()>> {
        let (source, layer, feature, _) = Self::selector_views(selector);
        let state = maplibre_core::string::buffer_view(state);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_set_feature_state_start(
                self.inner.native()?,
                source,
                layer,
                feature,
                state,
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderFeatureState)
    }

    pub fn get_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<crate::runtime::OperationHandle<Vec<u8>>> {
        let (source, layer, feature, _) = Self::selector_views(selector);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state_start(
                self.inner.native()?,
                source,
                layer,
                feature,
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderFeatureState)
    }

    pub fn remove_feature_state(
        &self,
        selector: &FeatureStateSelector,
    ) -> Result<crate::runtime::OperationHandle<()>> {
        let (source, layer, feature, key) = Self::selector_views(selector);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_remove_feature_state_start(
                self.inner.native()?,
                source,
                layer,
                feature,
                key,
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderFeatureState)
    }

    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<crate::runtime::OperationHandle<Vec<u8>>> {
        let geometry = geometry.to_native();
        let options = options
            .map(RenderedFeatureQueryOptions::to_native)
            .transpose()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_rendered_features_start(
                self.inner.native()?,
                geometry.as_ptr(),
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeRenderedFeatureQueryOptions::as_ptr),
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderQuery)
    }

    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<crate::runtime::OperationHandle<Vec<u8>>> {
        let source_id = maplibre_core::string::buffer_view(source_id.as_bytes());
        let options = options
            .map(SourceFeatureQueryOptions::to_native)
            .transpose()?;
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_source_features_start(
                self.inner.native()?,
                source_id,
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeSourceFeatureQueryOptions::as_ptr),
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderQuery)
    }

    pub fn query_feature_extension(
        &self,
        source_id: &str,
        feature: &[u8],
        extension: &str,
        extension_field: &str,
        arguments: Option<&[u8]>,
    ) -> Result<crate::runtime::OperationHandle<Vec<u8>>> {
        let source_id = maplibre_core::string::buffer_view(source_id.as_bytes());
        let feature = maplibre_core::string::buffer_view(feature);
        let extension = maplibre_core::string::buffer_view(extension.as_bytes());
        let extension_field = maplibre_core::string::buffer_view(extension_field.as_bytes());
        let arguments = arguments.map(maplibre_core::string::buffer_view);
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_feature_extensions_start(
                self.inner.native()?,
                source_id,
                feature,
                extension,
                extension_field,
                arguments.as_ref().map_or(ptr::null(), ptr::from_ref),
                &mut operation,
            )
        })?;
        self.inner
            .operation(operation, crate::runtime::OperationKind::RenderQuery)
    }
}
