use std::ptr;

use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

use crate::Result;
use crate::geojson::{Feature, FeatureNativeExt};
use crate::json::{JsonValue, JsonValueNativeExt};

pub use maplibre_core::query::{
    FeatureExtensionResult, FeatureStateSelector, QueriedFeature, RenderedFeatureQueryOptions,
    RenderedQueryGeometry, SourceFeatureQueryOptions,
};
pub(crate) use maplibre_core::query::{
    FeatureStateSelectorNativeExt, NativeRenderedFeatureQueryOptions,
    NativeSourceFeatureQueryOptions, RenderedFeatureQueryOptionsNativeExt,
    RenderedQueryGeometryNativeExt, SourceFeatureQueryOptionsNativeExt,
};

impl super::RenderSessionHandle {
    /// Sets per-feature state on a render source for this session.
    pub fn set_feature_state(
        &self,
        selector: &FeatureStateSelector,
        state: &JsonValue,
    ) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        let state = state.try_to_native()?;
        // SAFETY: session is live. selector and state own all call-scoped
        // descriptor storage until the native call returns.
        maplibre_core::check(unsafe {
            sys::mln_render_session_set_feature_state(session, selector.as_ptr(), state.as_ptr())
        })
    }

    /// Copies per-feature state from a render source in this session.
    pub fn get_feature_state(&self, selector: &FeatureStateSelector) -> Result<JsonValue> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_json_snapshot>::new();
        // SAFETY: session is live, selector owns call-scoped storage, and out
        // is a null-initialized out-pointer owned by this call.
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state(session, selector.as_ptr(), out.as_mut_ptr())
        })?;
        // SAFETY: On success, the C API returns either null or an owned JSON
        // snapshot handle for this call; core copies and releases it.
        Ok(
            unsafe { maplibre_core::json::copy_json_snapshot(out.into_option()) }?
                .unwrap_or_else(|| JsonValue::Object(Vec::new())),
        )
    }

    /// Copies per-feature state from a render source in this session.
    pub fn feature_state(&self, selector: &FeatureStateSelector) -> Result<JsonValue> {
        self.get_feature_state(selector)
    }

    /// Removes per-feature state selected for this session.
    pub fn remove_feature_state(&self, selector: &FeatureStateSelector) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let selector = selector.to_native();
        // SAFETY: session is live and selector owns all call-scoped storage
        // until the native call returns.
        maplibre_core::check(unsafe {
            sys::mln_render_session_remove_feature_state(session, selector.as_ptr())
        })
    }

    /// Queries rendered features from the latest render session state.
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<Vec<QueriedFeature>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let geometry = geometry.to_native();
        let options = options
            .map(RenderedFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_feature_query_result>::new();
        // SAFETY: session is live; geometry and options retain all borrowed
        // descriptor storage for the call; out is a null-initialized owned
        // out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_rendered_features(
                session,
                geometry.as_ptr(),
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeRenderedFeatureQueryOptions::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        // SAFETY: On success, the C API returns an owned feature-query result
        // handle; core copies and releases it.
        unsafe {
            maplibre_core::query::copy_feature_query_result(
                out.into_non_null("mln_feature_query_result")?,
            )
        }
    }

    /// Queries source features from the latest render session state.
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<Vec<QueriedFeature>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let options = options
            .map(SourceFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_feature_query_result>::new();
        // SAFETY: session is live; source_id and options retain all borrowed
        // descriptor storage for the call; out is a null-initialized owned
        // out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_source_features(
                session,
                source_id.raw(),
                options
                    .as_ref()
                    .map_or(ptr::null(), NativeSourceFeatureQueryOptions::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        // SAFETY: On success, the C API returns an owned feature-query result
        // handle; core copies and releases it.
        unsafe {
            maplibre_core::query::copy_feature_query_result(
                out.into_non_null("mln_feature_query_result")?,
            )
        }
    }

    /// Queries a feature extension from the latest render session state.
    pub fn query_feature_extension(
        &self,
        source_id: &str,
        feature: &Feature,
        extension: &str,
        extension_field: &str,
        arguments: Option<&JsonValue>,
    ) -> Result<FeatureExtensionResult> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let extension = maplibre_core::string::string_view(extension);
        let extension_field = maplibre_core::string::string_view(extension_field);
        let feature = feature.try_to_native(0)?;
        if let Some(arguments) = arguments
            && !matches!(arguments, JsonValue::Object(_))
        {
            return Err(crate::Error::invalid_argument(
                "feature extension arguments must be a JSON object",
            ));
        }
        let arguments = arguments.map(JsonValue::try_to_native).transpose()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_feature_extension_result>::new();
        // SAFETY: session is live; all string, feature, and optional JSON
        // descriptors retain borrowed storage for the call; out is a
        // null-initialized owned out-pointer.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_feature_extensions(
                session,
                source_id.raw(),
                feature.as_ptr(),
                extension.raw(),
                extension_field.raw(),
                arguments
                    .as_ref()
                    .map_or(ptr::null(), crate::json::NativeJsonValue::as_ptr),
                out.as_mut_ptr(),
            )
        })?;
        // SAFETY: On success, the C API returns an owned feature-extension
        // result handle; core copies and releases it.
        unsafe {
            maplibre_core::query::copy_feature_extension_result(
                out.into_non_null("mln_feature_extension_result")?,
            )
        }
    }
}
