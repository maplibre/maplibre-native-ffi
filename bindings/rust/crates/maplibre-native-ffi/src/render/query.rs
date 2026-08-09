use std::ptr;

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::Result;

pub use maplibre_core::query::{
    FeatureStateSelector, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
pub(crate) use maplibre_core::query::{
    FeatureStateSelectorNativeExt, NativeRenderedFeatureQueryOptions,
    NativeSourceFeatureQueryOptions, RenderedFeatureQueryOptionsNativeExt,
    RenderedQueryGeometryNativeExt, SourceFeatureQueryOptionsNativeExt,
};

impl super::RenderSessionHandle {
    /// Sets per-feature state on a render source for this session.
    pub fn set_feature_state(&self, selector: &FeatureStateSelector, state: &[u8]) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let selector = selector.to_native();
        let state = maplibre_core::string::buffer_view(state);
        // SAFETY: session is live and all borrowed storage remains valid for the call.
        maplibre_core::check(unsafe {
            sys::mln_render_session_set_feature_state(session, selector.as_ptr(), state)
        })
    }

    /// Copies per-feature state from a render source in this session.
    pub fn get_feature_state(&self, selector: &FeatureStateSelector) -> Result<Vec<u8>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let selector = selector.to_native();
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: session is live, selector storage remains valid, and out is writable.
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_feature_state(session, selector.as_ptr(), out.as_mut_ptr())
        })?;
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
    }

    /// Removes per-feature state selected for this session.
    pub fn remove_feature_state(&self, selector: &FeatureStateSelector) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let selector = selector.to_native();
        // SAFETY: session is live and selector storage remains valid for the call.
        maplibre_core::check(unsafe {
            sys::mln_render_session_remove_feature_state(session, selector.as_ptr())
        })
    }

    /// Queries rendered features as a UTF-8 JSON array.
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<Vec<u8>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let geometry = geometry.to_native();
        let options = options
            .map(RenderedFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: All descriptors and out storage remain valid for the call.
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
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
    }

    /// Queries source features as a UTF-8 JSON array.
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<Vec<u8>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let options = options
            .map(SourceFeatureQueryOptions::to_native)
            .transpose()?;
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: All descriptors and out storage remain valid for the call.
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
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
    }

    /// Queries a feature extension as UTF-8 JSON or GeoJSON bytes.
    pub fn query_feature_extension(
        &self,
        source_id: &str,
        feature: &[u8],
        extension: &str,
        extension_field: &str,
        arguments: Option<&[u8]>,
    ) -> Result<Vec<u8>> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let source_id = maplibre_core::string::string_view(source_id);
        let feature = maplibre_core::string::buffer_view(feature);
        let extension = maplibre_core::string::string_view(extension);
        let extension_field = maplibre_core::string::string_view(extension_field);
        let arguments = arguments.map(maplibre_core::string::buffer_view);
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_buffer>::new();
        // SAFETY: All views and out storage remain valid for the call.
        maplibre_core::check(unsafe {
            sys::mln_render_session_query_feature_extensions(
                session,
                source_id.raw(),
                feature,
                extension.raw(),
                extension_field.raw(),
                arguments.as_ref().map_or(ptr::null(), ptr::from_ref),
                out.as_mut_ptr(),
            )
        })?;
        // SAFETY: Success transfers the owned buffer to this call.
        unsafe { maplibre_core::string::copy_owned_buffer(out.get()) }
    }
}
