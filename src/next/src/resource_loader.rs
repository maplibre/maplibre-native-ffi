// Ports maplibre-native-ffi src/resources/resource_loader.cpp (the network
// file source that hands a runtime's network requests to its resource
// provider, and explains the URL schemes that nothing can load),
// src/resources/file_source_manager.cpp (the file source factories) and the
// platform context registry of src/runtime/runtime.cpp.
//
// A runtime's ResourceOptions carry a platform context token, which the
// FileSourceManager keys its file sources by, so every runtime has file
// sources of its own. The file sources find the runtime's resource provider
// and transforms through the token. Tokens are never reused, so a file
// source that outlives its runtime never names a later one.

use crate::database_file_source::DatabaseFileSource;
use crate::delivery::{UserData, deliver};
use crate::http_headers::HeaderTransform;
use crate::online_file_source::OnlineFileSource;
use crate::resource_provider::{ProviderRegistration, request_custom_resource};
use crate::types::*;
use maplibre_native_next::maplibre_native_base::Value;
use maplibre_native_next::mln_storage::file_source::Callback;
use maplibre_native_next::mln_storage::resource_transform::ResourceTransform;
use maplibre_native_next::mln_storage::response::{Error, Reason};
use maplibre_native_next::mln_storage::{
    AsyncRequest, ClientOptions, FileSource, FileSourceManager, FileSourceRequest, FileSourceType, Kind, LoadingMethod,
    Resource, ResourceOptions, Response,
};
use maplibre_native_next::mln_util::mapbox;
use std::collections::{HashMap, HashSet};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, LazyLock, Mutex, Once, Weak};

/// A callback registration that a runtime holds: its callback, user_data and
/// delivery queue. Dropping the last reference releases the user_data, so a
/// call in flight keeps it alive until it returns (C++
/// `RuntimeCallbackContext` and the registration leases).
pub struct Registration<C> {
    pub callback: C,
    pub user_data: UserData,
    release_user_data: mln_user_data_release,
    pub queue: u64,
}

impl<C> Registration<C> {
    pub fn new(callback: C, user_data: UserData, release_user_data: mln_user_data_release, queue: u64) -> Self {
        Registration { callback, user_data, release_user_data, queue }
    }
}

impl<C> Drop for Registration<C> {
    fn drop(&mut self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(
                self.queue,
                Box::new(move || {
                    // SAFETY: the registration's release, called once.
                    unsafe { release(user_data.get()) };
                }),
            );
        }
    }
}

pub type TransformRegistration = Registration<mln_resource_transform_callback>;

/// A runtime's resource configuration, which its file sources read from
/// their threads (C++ `ResourceProviderState`, `ResourceTransformState`,
/// `HttpHeaderTransformState` and `OfflineRegionEventState`).
pub struct ResourceState {
    pub token: usize,
    /// The runtime, which raises the offline region events.
    pub runtime: u64,
    provider: Mutex<Option<Arc<ProviderRegistration>>>,
    transform: Mutex<Option<Arc<TransformRegistration>>>,
    header_transform: Mutex<Option<Arc<HeaderTransform>>>,
    /// The offline regions whose events the runtime reports; `None` once the
    /// runtime is gone.
    pub observed_regions: Mutex<Option<HashSet<i64>>>,
}

static REGISTRY: LazyLock<Mutex<HashMap<usize, Weak<ResourceState>>>> = LazyLock::new(|| Mutex::new(HashMap::new()));
static NEXT_TOKEN: AtomicUsize = AtomicUsize::new(1);

impl ResourceState {
    pub fn new(runtime: u64) -> Arc<ResourceState> {
        let token = NEXT_TOKEN.fetch_add(1, Ordering::Relaxed);
        let state = Arc::new(ResourceState {
            token,
            runtime,
            provider: Mutex::new(None),
            transform: Mutex::new(None),
            header_transform: Mutex::new(None),
            observed_regions: Mutex::new(Some(HashSet::new())),
        });
        REGISTRY.lock().unwrap().insert(token, Arc::downgrade(&state));
        state
    }

    pub fn provider(&self) -> Option<Arc<ProviderRegistration>> {
        self.provider.lock().unwrap().clone()
    }

    pub fn transform(&self) -> Option<Arc<TransformRegistration>> {
        self.transform.lock().unwrap().clone()
    }

    pub fn header_transform(&self) -> Option<Arc<HeaderTransform>> {
        self.header_transform.lock().unwrap().clone()
    }

    /// Each `replace_*` returns the registration it retires, for the caller
    /// to drop outside the lock.
    pub fn replace_provider(&self, next: Option<Arc<ProviderRegistration>>) -> Option<Arc<ProviderRegistration>> {
        std::mem::replace(&mut *self.provider.lock().unwrap(), next)
    }

    pub fn replace_transform(&self, next: Option<Arc<TransformRegistration>>) -> Option<Arc<TransformRegistration>> {
        std::mem::replace(&mut *self.transform.lock().unwrap(), next)
    }

    pub fn replace_header_transform(&self, next: Option<Arc<HeaderTransform>>) -> Option<Arc<HeaderTransform>> {
        std::mem::replace(&mut *self.header_transform.lock().unwrap(), next)
    }

    /// The runtime is closing: no file source reaches it again.
    pub fn unregister(&self) {
        REGISTRY.lock().unwrap().remove(&self.token);
    }

    /// C++ `release_runtime_reachable_state`: the runtime is gone, so its
    /// registrations retire.
    pub fn retire(&self) {
        self.unregister();
        let provider = self.replace_provider(None);
        let transform = self.replace_transform(None);
        let header_transform = self.replace_header_transform(None);
        self.observed_regions.lock().unwrap().take();
        drop((provider, transform, header_transform));
    }
}

impl Drop for ResourceState {
    fn drop(&mut self) {
        let mut registry = REGISTRY.lock().unwrap();
        if registry.get(&self.token).is_some_and(|state| state.strong_count() == 0) {
            registry.remove(&self.token);
        }
    }
}

/// The resource configuration of the runtime whose platform context is
/// `token`, while that runtime is open.
pub fn resource_state(token: usize) -> Option<Arc<ResourceState>> {
    if token == 0 {
        return None;
    }
    REGISTRY.lock().unwrap().get(&token).and_then(Weak::upgrade)
}

/// `has_resource_transform_for_platform_context`.
pub fn has_resource_transform(token: usize) -> bool {
    resource_state(token).is_some_and(|state| state.transform().is_some())
}

/// `FileSourceManager::get()` with the C API's factories: the network file
/// source that serves resource providers, and the database file source with
/// the offline region API.
pub fn install_file_source_factories() {
    static INSTALLED: Once = Once::new();
    INSTALLED.call_once(|| {
        let manager = FileSourceManager::get();
        manager.register_file_source_factory(
            FileSourceType::Network,
            Arc::new(|resource_options: &ResourceOptions, client_options: &ClientOptions| {
                Arc::new(AbiNetworkFileSource::new(resource_options, client_options)) as Arc<dyn FileSource>
            }),
        );
        manager.register_file_source_factory(
            FileSourceType::Database,
            Arc::new(|resource_options: &ResourceOptions, client_options: &ClientOptions| {
                Arc::new(DatabaseFileSource::new(resource_options, client_options)) as Arc<dyn FileSource>
            }),
        );
    });
}

/// `resource_kind_to_abi`: the core's kinds have the C API's values.
pub fn kind_to_abi(kind: Kind) -> u32 {
    kind as u32
}

pub fn error_response(reason: Reason, message: impl Into<String>) -> Response {
    let mut response = Response::new();
    response.error = Some(Box::new(Error::new(reason, message)));
    response
}

/// `respond_immediately`: answers `callback` from the calling thread's run
/// loop, as a file source answers.
pub fn respond_immediately(response: Response, callback: Callback) -> Box<dyn AsyncRequest> {
    let request = FileSourceRequest::new(callback);
    request.actor().invoke(move |state| state.set_response(response));
    Box::new(request)
}

/// `url_scheme`: an RFC 3986 scheme starts with a letter and continues with
/// letters, digits, '+', '-' or '.'. A colon in a path alone names none.
fn url_scheme(url: &str) -> Option<&str> {
    let separator = url.find(':')?;
    let scheme = &url[..separator];
    let first = scheme.chars().next()?;
    if !first.is_ascii_alphabetic() {
        return None;
    }
    scheme
        .chars()
        .all(|character| character.is_ascii_alphanumeric() || matches!(character, '+' | '-' | '.'))
        .then_some(scheme)
}

/// `diagnostic_url`: the URL without its query, fragment and RFC 3986
/// userinfo, so a diagnostic never carries credentials.
fn diagnostic_url(url: &str) -> String {
    let suffix = [url.find('?'), url.find('#')].into_iter().flatten().min().unwrap_or(url.len());
    let mut sanitized = url[..suffix].to_owned();
    let Some(scheme_separator) = sanitized.find(':') else {
        return sanitized;
    };
    if !sanitized[scheme_separator + 1..].starts_with("//") {
        return sanitized;
    }
    let authority_start = scheme_separator + 3;
    let authority_end = sanitized[authority_start..].find('/').map_or(sanitized.len(), |end| authority_start + end);
    if let Some(at) = sanitized[..authority_end].rfind('@')
        && at >= authority_start
    {
        sanitized.replace_range(authority_start..=at, "");
    }
    sanitized
}

fn unsupported_scheme_response(scheme: &str, url: &str, provider_declined: bool) -> Response {
    // Reason::Other makes this terminal; the transport and server reasons are
    // retried or absorbed by the tile and source paths.
    error_response(
        Reason::Other,
        format!(
            "unsupported URL scheme \"{scheme}\" for network request \"{}{}",
            diagnostic_url(url),
            if provider_declined {
                "\"; the registered resource provider declined this request; update it to serve this scheme"
            } else {
                "\"; register a resource provider with mln_runtime_set_resource_provider() to serve this scheme"
            }
        ),
    )
}

/// `resolve_resource_url`: the URL that the built-in network path would
/// request. A URL that normalization rejects stays as the request names it,
/// so a provider can still serve it.
pub fn resolve_resource_url(options: &ResourceOptions, resource: &Resource) -> String {
    let tile_server = options.tile_server_options();
    let api_key = options.api_key();
    match resource.kind {
        Kind::Style => mapbox::normalize_style_url(&tile_server, &resource.url, api_key),
        Kind::Source => {
            mapbox::normalize_source_url(&tile_server, &resource.url, api_key).unwrap_or_else(|_| resource.url.clone())
        }
        Kind::Glyphs => mapbox::normalize_glyphs_url(&tile_server, &resource.url, api_key),
        Kind::SpriteImage | Kind::SpriteJSON => mapbox::normalize_sprite_url(&tile_server, &resource.url, api_key),
        Kind::Tile => mapbox::normalize_tile_url(&tile_server, &resource.url, api_key),
        Kind::Unknown | Kind::Image => resource.url.clone(),
    }
}

/// `AbiNetworkFileSource`: the `Network` file source of a runtime. A
/// request that the runtime's resource provider routes goes to the
/// provider; the rest load through the online file source.
pub struct AbiNetworkFileSource {
    resource_options: Mutex<ResourceOptions>,
    client_options: Mutex<ClientOptions>,
    native: Arc<dyn FileSource>,
}

impl AbiNetworkFileSource {
    pub fn new(resource_options: &ResourceOptions, client_options: &ClientOptions) -> Self {
        AbiNetworkFileSource {
            resource_options: Mutex::new(resource_options.clone()),
            client_options: Mutex::new(client_options.clone()),
            native: Arc::new(OnlineFileSource::new(resource_options, client_options)),
        }
    }

    fn options(&self) -> ResourceOptions {
        self.resource_options.lock().unwrap().clone()
    }

    /// The provider that serves `resource`, and whether one is registered.
    fn route(
        &self,
        options: &ResourceOptions,
        resource: &Resource,
    ) -> (Option<(Arc<ProviderRegistration>, String)>, bool) {
        if !resource.has_loading_method(LoadingMethod::Network) {
            return (None, false);
        }
        let Some(provider) = resource_state(options.platform_context()).and_then(|state| state.provider()) else {
            return (None, false);
        };
        let resolved_url = resolve_resource_url(options, resource);
        // A request outside the provider's routes passes through, as one the
        // provider declined does.
        if provider.matches(kind_to_abi(resource.kind), &resolved_url) {
            (Some((provider, resolved_url)), true)
        } else {
            (None, true)
        }
    }
}

/// `request_native`.
fn request_native(
    native: &Arc<dyn FileSource>,
    options: &ResourceOptions,
    resource: &Resource,
    provider_declined: bool,
    callback: Callback,
) -> Box<dyn AsyncRequest> {
    if !native.can_request(resource) {
        return respond_immediately(
            error_response(Reason::Other, "native loader cannot request this resource"),
            callback,
        );
    }
    if let Some(scheme) = unsupported_network_scheme(options, &resource.url) {
        return respond_immediately(unsupported_scheme_response(scheme, &resource.url, provider_declined), callback);
    }
    native.request(resource, callback)
}

/// `unsupported_network_scheme`: the scheme of a URL that the online file
/// source can't load. A resource transform rewrites URLs inside the online
/// file source, so it keeps every scheme available.
fn unsupported_network_scheme<'a>(options: &ResourceOptions, url: &'a str) -> Option<&'a str> {
    if has_resource_transform(options.platform_context()) {
        return None;
    }
    let scheme = url_scheme(url)?;
    if scheme.eq_ignore_ascii_case("http") || scheme.eq_ignore_ascii_case("https") {
        return None;
    }
    // Tile server options name a canonical scheme that the online file
    // source expands into its base URL.
    let alias = options.tile_server_options().uri_scheme_alias().to_owned();
    if !alias.is_empty() && scheme.eq_ignore_ascii_case(&alias) {
        return None;
    }
    Some(scheme)
}

impl FileSource for AbiNetworkFileSource {
    fn request(&self, resource: &Resource, callback: Callback) -> Box<dyn AsyncRequest> {
        let options = self.options();
        let (route, provider_declined) = self.route(&options, resource);
        if let Some((provider, resolved_url)) = route {
            let native = self.native.clone();
            let original = resource.clone();
            return request_custom_resource(
                resource,
                resolved_url,
                provider,
                callback,
                Box::new(move |next| {
                    let options = native.get_resource_options();
                    request_native(&native, &options, &original, true, next)
                }),
            );
        }
        request_native(&self.native, &options, resource, provider_declined, callback)
    }

    fn can_request(&self, resource: &Resource) -> bool {
        // A provider claims only the requests that its routes match, so the
        // rest stay available to another file source.
        let options = self.options();
        self.route(&options, resource).0.is_some() || self.native.can_request(resource)
    }

    fn forward(&self, resource: &Resource, response: &Response, callback: Option<Box<dyn FnOnce() + Send>>) {
        self.native.forward(resource, response, callback);
    }

    fn supports_cache_only_requests(&self) -> bool {
        self.native.supports_cache_only_requests()
    }

    fn pause(&self) {
        self.native.pause();
    }

    fn resume(&self) {
        self.native.resume();
    }

    fn set_resource_transform(&self, transform: ResourceTransform) {
        self.native.set_resource_transform(transform);
    }

    fn set_property(&self, key: &str, value: &Value) {
        self.native.set_property(key, value);
    }

    fn get_property(&self, key: &str) -> Value {
        self.native.get_property(key)
    }

    fn set_resource_options(&self, options: ResourceOptions) {
        *self.resource_options.lock().unwrap() = options.clone();
        self.native.set_resource_options(options);
    }

    fn get_resource_options(&self) -> ResourceOptions {
        self.options()
    }

    fn set_client_options(&self, options: ClientOptions) {
        *self.client_options.lock().unwrap() = options.clone();
        self.native.set_client_options(options);
    }

    fn get_client_options(&self) -> ClientOptions {
        self.client_options.lock().unwrap().clone()
    }
}

#[cfg(test)]
mod tests {
    use super::{diagnostic_url, url_scheme};

    #[test]
    fn diagnostics_drop_credentials_queries_and_fragments() {
        assert_eq!(
            diagnostic_url("jar://user:password@archive/packaged/style.json?access_token=secret#token"),
            "jar://archive/packaged/style.json"
        );
        assert_eq!(diagnostic_url("jar:file:/packaged/style.json"), "jar:file:/packaged/style.json");
    }

    #[test]
    fn a_scheme_starts_with_a_letter() {
        assert_eq!(url_scheme("jar:file:/x"), Some("jar"));
        assert_eq!(url_scheme("/a:b"), None);
        assert_eq!(url_scheme("1a:b"), None);
        assert_eq!(url_scheme(":b"), None);
    }
}
