// The online file source of maplibre-native-next's mln_storage
// (online_file_source.rs, which ports MapLibre Native's
// platform/default/src/mbgl/storage/online_file_source.cpp), with the hooks
// that maplibre-native-ffi's MapLibre build gives its online file source:
//
// - The resource transform is the runtime's (src/runtime/runtime.cpp
//   invoke_resource_transform), and it can fail a request without sending
//   it. A failed transform counts as a failed request: the request retries
//   on the schedule of its error's reason, and the retry calls the transform
//   again.
// - The HTTP header transform adds headers to each HTTP request
//   (invoke_http_header_transform). A request with added headers goes
//   through `header_transport` below, which follows redirects itself so that
//   the headers stay within the request's origin; the rest go through the
//   core's HTTPFileSource.
//
// The rest is the core's code: the requests run on the "OnlineFileSource"
// thread, scheduled by a timer (first request at once, then revalidation
// when the response expires, or retries with backoff after errors), queued
// while `maximumConcurrentRequests` transfers are active.

use crate::http_headers::invoke_http_header_transform;
use crate::resource_loader::{has_resource_transform, kind_to_abi};
use crate::resource_transform::invoke_resource_transform;
use maplibre_native_next::maplibre_native_base::Value;
use maplibre_native_next::mln_actor::{ActorRef, AsyncTask, Mailbox, Thread, Timer, scheduler};
use maplibre_native_next::mln_storage::async_request::AsyncRequest;
use maplibre_native_next::mln_storage::file_source::{
    API_BASE_URL_KEY, API_KEY_KEY, Callback, FileSource, MAX_CONCURRENT_REQUESTS_KEY,
};
use maplibre_native_next::mln_storage::file_source_request::{FileSourceRequest, FileSourceRequestState};
use maplibre_native_next::mln_storage::http_file_source::HTTPFileSource;
use maplibre_native_next::mln_storage::http_timeout::{error_retry_timeout, expiration_timeout};
use maplibre_native_next::mln_storage::network_status::{NetworkStatus, Status};
use maplibre_native_next::mln_storage::resource::{Kind, Priority, Resource};
use maplibre_native_next::mln_storage::resource_options::ResourceOptions;
use maplibre_native_next::mln_storage::response::{Error, Reason, Response};
use maplibre_native_next::mln_util::chrono::{Duration, Timestamp, now};
use maplibre_native_next::mln_util::client_options::ClientOptions;
use maplibre_native_next::mln_util::constants::{
    ASSET_PROTOCOL, CLOCK_SKEW_RETRY_TIMEOUT, DEFAULT_MAXIMUM_CONCURRENT_REQUESTS, FILE_PROTOCOL,
};
use maplibre_native_next::mln_util::logging::{Event, Log};
use maplibre_native_next::mln_util::mapbox;
use maplibre_native_next::mln_util::tile_server_options::TileServerOptions;
use std::cell::{Cell, RefCell};
use std::collections::{BTreeMap, BTreeSet, VecDeque};
use std::rc::{Rc, Weak};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

/// For testing only
pub const ONLINE_STATUS_KEY: &str = "online-status";

/// `OnlineFileRequest`
struct OnlineFileRequest {
    id: u64,
    impl_: Weak<ThreadState>,
    resource: RefCell<Resource>,
    request: RefCell<Option<Box<dyn AsyncRequest>>>,
    timer: Timer,
    callback: Box<dyn Fn(Response)>,

    /// Counts the number of times a response was already expired when
    /// received. We're using this to add a delay when making a new request
    /// so we don't keep retrying immediately in case of a server serving
    /// expired tiles.
    expired_requests: Cell<u32>,

    /// Counts the number of subsequent failed requests. We're using this
    /// value for exponential backoff when retrying requests.
    failed_requests: Cell<u32>,
    failed_request_reason: Cell<Reason>,
    retry_after: Cell<Option<Timestamp>>,

    /// The last transform of the URL failed, so the retry transforms it
    /// again.
    transform_failed: Cell<bool>,

    /// Lazily created for the ResourceTransform's reply (`actor()`).
    mailbox: RefCell<Option<Arc<Mailbox>>>,
}

/// The target of an `OnlineFileRequest`'s mailbox.
struct OnlineFileRequestRef {
    impl_: Weak<ThreadState>,
    id: u64,
}

impl OnlineFileRequestRef {
    /// `OnlineFileRequest::setTransformedURL(const std::string&)`, or the
    /// transform's failure.
    fn set_transformed_url(&self, url: Result<String, Error>) {
        if let Some(impl_) = self.impl_.upgrade()
            && let Some(req) = impl_.get(self.id)
        {
            req.set_transformed_url(url);
        }
    }
}

impl Drop for OnlineFileRequest {
    fn drop(&mut self) {
        if let Some(mailbox) = self.mailbox.borrow_mut().take() {
            mailbox.close();
        }
    }
}

/// `PendingRequests`: a FIFO preferring regular over low priority requests.
#[derive(Default)]
struct PendingRequests {
    regular: VecDeque<u64>,
    low: VecDeque<u64>,
}

impl PendingRequests {
    fn remove(&mut self, id: u64) {
        self.regular.retain(|&r| r != id);
        self.low.retain(|&r| r != id);
    }

    fn insert(&mut self, id: u64, priority: Priority) {
        if priority == Priority::Regular {
            self.regular.push_back(id);
        } else {
            self.low.push_back(id);
        }
    }

    fn pop(&mut self) -> Option<u64> {
        self.regular.pop_front().or_else(|| self.low.pop_front())
    }

    fn contains(&self, id: u64) -> bool {
        self.regular.contains(&id) || self.low.contains(&id)
    }
}

/// The members of `OnlineFileSourceThread`.
struct ThreadState {
    resource_options: RefCell<ResourceOptions>,
    client_options: RefCell<ClientOptions>,

    /// The lifetime of a request is:
    ///
    /// 1. Waiting for timeout (revalidation or retry)
    /// 2. Pending (waiting for room in the active set)
    /// 3. Active (open network connection)
    /// 4. Back to #1
    ///
    /// Requests in any state are in `all_requests`. Requests in the pending
    /// state are in `pending_requests`. Requests in the active state are in
    /// `active_requests`.
    all_requests: RefCell<BTreeSet<u64>>,
    pending_requests: RefCell<PendingRequests>,
    active_requests: RefCell<BTreeSet<u64>>,

    online: Cell<bool>,
    maximum_concurrent_requests: Cell<u32>,
    http_file_source: HTTPFileSource,
    /// The transfers that carry transformed headers, once there is one.
    header_file_source: RefCell<Option<header_transport::HeaderHTTPFileSource>>,
    tasks: RefCell<BTreeMap<u64, Rc<OnlineFileRequest>>>,
}

/// `OnlineFileSourceThread`
pub struct OnlineFileSourceThread {
    state: Rc<ThreadState>,
    _reachability: AsyncTask,
    reachability_id: u64,
}

/// `interpolateExpiration(current, prior, expired)`
fn interpolate_expiration(current: Timestamp, prior: Option<Timestamp>, expired: &mut bool) -> Timestamp {
    let now = now();
    if current > now {
        return current;
    }

    let Some(prior) = prior else {
        *expired = true;
        return current;
    };

    // Expiring date is going backwards, fallback to exponential backoff.
    if current < prior {
        *expired = true;
        return current;
    }

    let delta = current.secs_since_epoch() - prior.secs_since_epoch();

    // Server is serving the same expired resource over and over, fallback
    // to exponential backoff.
    if delta == 0 {
        *expired = true;
        return current;
    }

    // Assume that either the client or server clock is wrong and try to
    // interpolate a valid expiration date (from the client POV) observing a
    // minimum timeout.
    let minimum = CLOCK_SKEW_RETRY_TIMEOUT.count() / 1_000_000_000;
    Timestamp::from_secs(now.secs_since_epoch() + delta.max(minimum))
}

/// `sanitizeURL`: the URL without its query string.
fn sanitize_url(url: &str) -> &str {
    match url.find('?') {
        Some(query_index) => &url[..query_index],
        None => url,
    }
}

impl ThreadState {
    fn get(&self, id: u64) -> Option<Rc<OnlineFileRequest>> {
        self.tasks.borrow().get(&id).cloned()
    }

    /// The platform context of the runtime whose transforms apply.
    fn token(&self) -> usize {
        self.resource_options.borrow().platform_context()
    }

    /// `add(OnlineFileRequest*)`
    fn add(&self, req: &OnlineFileRequest) {
        self.all_requests.borrow_mut().insert(req.id);
        if has_resource_transform(self.token()) {
            self.transform(req);
        } else {
            req.activate();
        }
    }

    /// Request the ResourceTransform actor a new url and replace the
    /// resource url with the transformed one before proceeding to schedule
    /// the request.
    fn transform(&self, req: &OnlineFileRequest) {
        let r = req.actor();
        let (kind, url) = {
            let resource = req.resource.borrow();
            (resource.kind, resource.url.clone())
        };
        invoke_resource_transform(
            self.token(),
            kind_to_abi(kind),
            &url,
            Box::new(move |url: Result<String, Error>| {
                r.invoke(move |req: &OnlineFileRequestRef| req.set_transformed_url(url));
            }),
        );
    }

    /// `HTTPFileSource::request`, with the headers that the runtime's HTTP
    /// header transform adds.
    fn request_http(&self, resource: &Resource, callback: Callback) -> Box<dyn AsyncRequest> {
        let url = resource.url.as_str();
        let is_http = ["http://", "https://"]
            .iter()
            .any(|scheme| url.len() >= scheme.len() && url[..scheme.len()].eq_ignore_ascii_case(scheme));
        if is_http && !cfg!(target_os = "emscripten") {
            let headers = invoke_http_header_transform(self.token(), kind_to_abi(resource.kind), url);
            if !headers.is_empty() {
                let mut source = self.header_file_source.borrow_mut();
                let source = source.get_or_insert_with(header_transport::HeaderHTTPFileSource::new);
                return source.request(resource, headers, callback);
            }
        }
        self.http_file_source.request(resource, callback)
    }

    /// `remove(OnlineFileRequest*)`
    fn remove(self: &Rc<Self>, id: u64) {
        self.all_requests.borrow_mut().remove(&id);
        let was_active = self.active_requests.borrow_mut().remove(&id);
        if was_active {
            self.activate_pending_request();
        } else {
            self.pending_requests.borrow_mut().remove(id);
        }
    }

    /// `activateOrQueueRequest(OnlineFileRequest*)`
    fn activate_or_queue_request(self: &Rc<Self>, id: u64) {
        let Some(req) = self.get(id) else {
            return;
        };
        debug_assert!(self.all_requests.borrow().contains(&id));
        debug_assert!(!self.active_requests.borrow().contains(&id));
        debug_assert!(req.request.borrow().is_none());

        if self.active_requests.borrow().len() >= self.maximum_concurrent_requests.get() as usize {
            let priority = req.resource.borrow().priority;
            self.pending_requests.borrow_mut().insert(id, priority);
        } else {
            self.activate_request(&req);
        }
    }

    /// `activateRequest(OnlineFileRequest*)`
    fn activate_request(self: &Rc<Self>, req: &Rc<OnlineFileRequest>) {
        let weak = Rc::downgrade(self);
        let id = req.id;
        let callback = move |response: Response| {
            let Some(this) = weak.upgrade() else {
                return;
            };
            this.active_requests.borrow_mut().remove(&id);
            if let Some(req) = this.get(id) {
                // The transfer is done; dropping its request is a no-op.
                let finished = req.request.borrow_mut().take();
                drop(finished);
                req.completed(response);
            }
            this.activate_pending_request();
        };

        self.active_requests.borrow_mut().insert(req.id);

        if self.online.get() {
            let resource = req.resource.borrow().clone();
            let request = self.request_http(&resource, Box::new(callback));
            *req.request.borrow_mut() = Some(request);
        } else {
            let mut response = Response::new();
            response.error = Some(Box::new(Error::new(Reason::Connection, "Online connectivity is disabled.")));
            callback(response);
        }
    }

    /// `activatePendingRequest()`
    fn activate_pending_request(self: &Rc<Self>) {
        let next = self.pending_requests.borrow_mut().pop();
        if let Some(id) = next
            && let Some(req) = self.get(id)
        {
            self.activate_request(&req);
        }
    }

    fn is_pending(&self, id: u64) -> bool {
        self.pending_requests.borrow().contains(id)
    }

    fn is_active(&self, id: u64) -> bool {
        self.active_requests.borrow().contains(&id)
    }

    /// `networkIsReachableAgain()`
    fn network_is_reachable_again(&self) {
        let requests: Vec<Rc<OnlineFileRequest>> =
            self.all_requests.borrow().iter().filter_map(|id| self.get(*id)).collect();
        // Notify regular priority requests, then low priority requests.
        for priority in [Priority::Regular, Priority::Low] {
            for req in &requests {
                if req.resource.borrow().priority == priority {
                    req.network_is_reachable_again();
                }
            }
        }
    }
}

impl OnlineFileRequest {
    /// `setTransformedURL(const std::string&)`. A failed transform fails the
    /// request as a failed transfer does, and its retry transforms again.
    fn set_transformed_url(&self, url: Result<String, Error>) {
        match url {
            Ok(url) => {
                self.transform_failed.set(false);
                self.resource.borrow_mut().url = url;
                self.activate();
            }
            Err(error) => {
                self.transform_failed.set(true);
                let mut response = Response::new();
                response.error = Some(Box::new(error));
                self.completed(response);
            }
        }
    }

    /// `actor()`
    fn actor(&self) -> ActorRef<OnlineFileRequestRef> {
        let mut mailbox = self.mailbox.borrow_mut();
        // Lazy constructed because this can be costly and the
        // ResourceTransform is not used by many apps.
        let mailbox = mailbox.get_or_insert_with(|| {
            let target = Rc::new(OnlineFileRequestRef { impl_: self.impl_.clone(), id: self.id });
            Mailbox::with_target(&scheduler::get_current(), target)
        });
        ActorRef::new(mailbox)
    }

    /// `activate()`
    fn activate(&self) {
        // Force an immediate first request if we don't have an expiration time.
        let prior_expires = self.resource.borrow().prior_expires;
        let timeout = match prior_expires {
            Some(_) => self.get_update_interval(prior_expires),
            None => Duration::zero(),
        };
        self.schedule(timeout);
    }

    /// `schedule(Duration timeout)`
    fn schedule(&self, mut timeout: Duration) {
        let Some(impl_) = self.impl_.upgrade() else {
            return;
        };
        if impl_.is_pending(self.id) || impl_.is_active(self.id) {
            // There's already a request in progress; don't start another one.
            return;
        }

        if timeout == Duration::max() {
            return;
        }

        // Emulate a Connection error when the Offline mode is forced with a
        // really long timeout. The request will get re-triggered when the
        // NetworkStatus is set back to Online.
        if NetworkStatus::get() == Status::Offline {
            self.failed_request_reason.set(Reason::Connection);
            self.failed_requests.set(1);
            timeout = Duration::max();
        }

        let weak = self.impl_.clone();
        let id = self.id;
        self.timer.start(timeout, Duration::zero(), move || {
            if let Some(impl_) = weak.upgrade() {
                match impl_.get(id) {
                    Some(req) if req.transform_failed.get() => impl_.transform(&req),
                    _ => impl_.activate_or_queue_request(id),
                }
            }
        });
    }

    /// `getUpdateInterval(std::optional<Timestamp> expires)`
    fn get_update_interval(&self, expires: Option<Timestamp>) -> Duration {
        // Calculate a timeout that depends on how many consecutive errors
        // we've encountered, and on the expiration time, if present.
        let error_retry =
            error_retry_timeout(self.failed_request_reason.get(), self.failed_requests.get(), self.retry_after.get());
        let expiration = expiration_timeout(expires, self.expired_requests.get());
        let minimum = self.resource.borrow().minimum_update_interval;
        let expiration = if expiration < minimum { minimum } else { expiration };
        if expiration < error_retry { expiration } else { error_retry }
    }

    /// `completed(Response)`
    fn completed(&self, mut response: Response) {
        // If we didn't get various caching headers in the response, continue
        // using the previous values. Otherwise, update the previous values
        // to the new values.
        {
            let mut resource = self.resource.borrow_mut();
            if response.modified.is_none() {
                response.modified = resource.prior_modified;
            } else {
                resource.prior_modified = response.modified;
            }

            if response.not_modified && resource.prior_data.is_some() {
                // When the priorData field is set, it indicates that we had
                // to revalidate the request and that the requestor hasn't
                // gotten data yet. If we get a 304 response, this means that
                // we have send the cached data to give the requestor a
                // chance to actually obtain the data.
                response.data = resource.prior_data.take();
                response.not_modified = false;
            }
        }

        let mut is_expired = false;

        if let Some(expires) = response.expires {
            let prior = self.resource.borrow().prior_expires;
            self.resource.borrow_mut().prior_expires = Some(expires);
            response.expires = Some(interpolate_expiration(expires, prior, &mut is_expired));
        }

        if is_expired {
            self.expired_requests.set(self.expired_requests.get() + 1);
        } else {
            self.expired_requests.set(0);
        }

        {
            let mut resource = self.resource.borrow_mut();
            if response.etag.is_none() {
                response.etag = resource.prior_etag.clone();
            } else {
                resource.prior_etag = response.etag.clone();
            }
        }

        if let Some(error) = &response.error {
            if error.reason == Reason::NotFound {
                let url = self.resource.borrow().url.clone();
                Log::error(Event::General, format!("The resource `{}` not found", sanitize_url(&url)));
            }
            self.failed_requests.set(self.failed_requests.get() + 1);
            self.failed_request_reason.set(error.reason);
            self.retry_after.set(error.retry_after);
        } else {
            self.failed_requests.set(0);
            self.failed_request_reason.set(Reason::Success);
        }

        self.schedule(self.get_update_interval(response.expires));

        // Calling the callback may result in `this` being deleted. It needs
        // to be done last.
        (self.callback)(response);
    }

    /// `networkIsReachableAgain()`
    fn network_is_reachable_again(&self) {
        // We need all requests to fail at least once before we are going to
        // start retrying them, and we only immediately restart request that
        // failed due to connection issues.
        if self.failed_request_reason.get() == Reason::Connection {
            self.schedule(Duration::zero());
        }
    }
}

impl OnlineFileSourceThread {
    fn new(resource_options: ResourceOptions, client_options: ClientOptions) -> Self {
        let http_file_source = HTTPFileSource::new(&resource_options, &client_options);
        let state = Rc::new(ThreadState {
            resource_options: RefCell::new(resource_options),
            client_options: RefCell::new(client_options),
            all_requests: RefCell::new(BTreeSet::new()),
            pending_requests: RefCell::new(PendingRequests::default()),
            active_requests: RefCell::new(BTreeSet::new()),
            online: Cell::new(true),
            maximum_concurrent_requests: Cell::new(DEFAULT_MAXIMUM_CONCURRENT_REQUESTS),
            http_file_source,
            header_file_source: RefCell::new(None),
            tasks: RefCell::new(BTreeMap::new()),
        });
        let weak = Rc::downgrade(&state);
        let reachability = AsyncTask::new(move || {
            if let Some(state) = weak.upgrade() {
                state.network_is_reachable_again();
            }
        });
        let reachability_id = NetworkStatus::subscribe(reachability.sender());
        OnlineFileSourceThread { state, _reachability: reachability, reachability_id }
    }

    /// `request(AsyncRequest* req, Resource, const ActorRef<FileSourceRequest>&)`
    pub fn request(&self, req: u64, resource: Resource, r: &ActorRef<FileSourceRequestState>) {
        let r = r.clone();
        let callback = move |res: Response| {
            r.invoke(move |request: &FileSourceRequestState| request.set_response(res));
        };
        let request = Rc::new(OnlineFileRequest {
            id: req,
            impl_: Rc::downgrade(&self.state),
            resource: RefCell::new(resource),
            request: RefCell::new(None),
            timer: Timer::new(),
            callback: Box::new(callback),
            expired_requests: Cell::new(0),
            failed_requests: Cell::new(0),
            failed_request_reason: Cell::new(Reason::Success),
            retry_after: Cell::new(None),
            transform_failed: Cell::new(false),
            mailbox: RefCell::new(None),
        });
        self.state.tasks.borrow_mut().insert(req, request.clone());
        self.state.add(&request);
    }

    /// `cancel(AsyncRequest* req)`
    pub fn cancel(&self, req: u64) {
        self.state.remove(req);
        let task = self.state.tasks.borrow_mut().remove(&req);
        drop(task);
    }

    fn set_resource_options(&self, options: ResourceOptions) {
        *self.state.resource_options.borrow_mut() = options;
    }

    fn set_client_options(&self, options: ClientOptions) {
        *self.state.client_options.borrow_mut() = options;
    }

    /// `setOnlineStatus(bool)`
    fn set_online_status(&self, status: bool) {
        self.state.online.set(status);
        if status {
            self.state.network_is_reachable_again();
        }
    }

    fn set_maximum_concurrent_requests(&self, maximum: u32) {
        self.state.maximum_concurrent_requests.set(maximum);
    }

    fn set_api_base_url(&self, url: String) {
        let mut options = self.state.resource_options.borrow_mut();
        let updated = options.clone().with_tile_server_options(TileServerOptions::new().with_base_url(url));
        *options = updated;
    }

    fn set_api_key(&self, key: String) {
        let mut options = self.state.resource_options.borrow_mut();
        let updated = options.clone().with_api_key(key);
        *options = updated;
    }
}

impl Drop for OnlineFileSourceThread {
    fn drop(&mut self) {
        NetworkStatus::unsubscribe(self.reachability_id);
        // Requests hold timers on this thread; drop them here.
        self.state.tasks.borrow_mut().clear();
    }
}

/// `OnlineFileSource::Impl`. `thread` is declared last, as in C++, but
/// dropped explicitly first (see `Drop`).
struct Impl {
    cached_resource_options: Mutex<ResourceOptions>,
    cached_client_options: Mutex<ClientOptions>,
    cached_maximum_concurrent_requests: Mutex<u32>,
    thread: Thread<OnlineFileSourceThread>,
}

static NEXT_REQUEST_ID: AtomicU64 = AtomicU64::new(1);

impl Impl {
    fn new(resource_options: &ResourceOptions, client_options: &ClientOptions) -> Self {
        let (ro, co) = (resource_options.clone(), client_options.clone());
        // TODO(port): makeThreadPrioritySetter(EXPERIMENTAL_THREAD_PRIORITY_NETWORK).
        let thread = Thread::new("OnlineFileSource", move |_| OnlineFileSourceThread::new(ro, co));
        Impl {
            cached_resource_options: Mutex::new(resource_options.clone()),
            cached_client_options: Mutex::new(client_options.clone()),
            cached_maximum_concurrent_requests: Mutex::new(DEFAULT_MAXIMUM_CONCURRENT_REQUESTS),
            thread,
        }
    }

    fn request(&self, callback: Callback, res: Resource) -> Box<dyn AsyncRequest> {
        let mut req = FileSourceRequest::new(callback);
        let req_id = NEXT_REQUEST_ID.fetch_add(1, Ordering::Relaxed);
        let actor_ref = self.thread.actor();
        req.on_cancel(Box::new(move || {
            actor_ref.invoke(move |t: &OnlineFileSourceThread| t.cancel(req_id));
        }));
        let req_ref = req.actor();
        self.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.request(req_id, res, &req_ref));
        Box::new(req)
    }

    fn get_api_key(&self) -> String {
        self.cached_resource_options.lock().unwrap().api_key().to_owned()
    }
}

/// `OnlineFileSource`: the `Network` file source.
pub struct OnlineFileSource {
    impl_: Impl,
}

impl OnlineFileSource {
    /// `OnlineFileSource(const ResourceOptions&, const ClientOptions&)`
    pub fn new(resource_options: &ResourceOptions, client_options: &ClientOptions) -> Self {
        OnlineFileSource { impl_: Impl::new(resource_options, client_options) }
    }
}

impl FileSource for OnlineFileSource {
    fn request(&self, resource: &Resource, callback: Callback) -> Box<dyn AsyncRequest> {
        let mut res = resource.clone();
        let options = self.impl_.cached_resource_options.lock().unwrap().tile_server_options();
        let api_key = self.impl_.get_api_key();

        match resource.kind {
            Kind::Unknown | Kind::Image => {}
            Kind::Style => res.url = mapbox::normalize_style_url(&options, &resource.url, &api_key),
            Kind::Source => match mapbox::normalize_source_url(&options, &resource.url, &api_key) {
                Ok(url) => res.url = url,
                // C++ throws out of `request`; the port answers with the error.
                Err(message) => {
                    let mut response = Response::new();
                    response.error = Some(Box::new(Error::new(Reason::Other, message)));
                    let req = FileSourceRequest::new(callback);
                    let r = req.actor();
                    r.invoke(move |request: &FileSourceRequestState| request.set_response(response));
                    return Box::new(req);
                }
            },
            Kind::Glyphs => res.url = mapbox::normalize_glyphs_url(&options, &resource.url, &api_key),
            Kind::SpriteImage | Kind::SpriteJSON => {
                res.url = mapbox::normalize_sprite_url(&options, &resource.url, &api_key)
            }
            Kind::Tile => res.url = mapbox::normalize_tile_url(&options, &resource.url, &api_key),
        }

        self.impl_.request(callback, res)
    }

    fn can_request(&self, resource: &Resource) -> bool {
        resource.has_loading_method(maplibre_native_next::mln_storage::resource::LoadingMethod::Network)
            && !resource.url.starts_with(ASSET_PROTOCOL)
            && !resource.url.starts_with(FILE_PROTOCOL)
    }

    fn pause(&self) {
        self.impl_.thread.pause();
    }

    fn resume(&self) {
        self.impl_.thread.resume();
    }

    fn set_property(&self, key: &str, value: &Value) {
        if key == API_KEY_KEY {
            if let Value::String(api_key) = value {
                let key = api_key.clone();
                self.impl_.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.set_api_key(key));
                let mut options = self.impl_.cached_resource_options.lock().unwrap();
                *options = options.clone().with_api_key(api_key.clone());
            } else {
                Log::error(Event::General, "Invalid apiKey property value type.");
            }
        } else if key == API_BASE_URL_KEY {
            if let Value::String(base_url) = value {
                let url = base_url.clone();
                self.impl_.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.set_api_base_url(url));
                let mut options = self.impl_.cached_resource_options.lock().unwrap();
                let tile_server_options = options.tile_server_options().with_base_url(base_url.clone());
                *options = options.clone().with_tile_server_options(tile_server_options);
            } else {
                Log::error(Event::General, "Invalid base-url property value type.");
            }
        } else if key == MAX_CONCURRENT_REQUESTS_KEY {
            if let Value::Uint(maximum) = value {
                debug_assert!(*maximum < u64::from(u32::MAX));
                let maximum = *maximum as u32;
                self.impl_
                    .thread
                    .actor()
                    .invoke(move |t: &OnlineFileSourceThread| t.set_maximum_concurrent_requests(maximum));
                *self.impl_.cached_maximum_concurrent_requests.lock().unwrap() = maximum;
            } else {
                Log::error(Event::General, "Invalid max-concurrent-requests property value type.");
            }
        } else if key == ONLINE_STATUS_KEY {
            // For testing only
            if let Value::Bool(status) = value {
                let status = *status;
                self.impl_.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.set_online_status(status));
            }
        } else {
            Log::error(Event::General, format!("Resource provider does not support property {key}"));
        }
    }

    fn get_property(&self, key: &str) -> Value {
        if key == API_KEY_KEY {
            Value::String(self.impl_.get_api_key())
        } else if key == API_BASE_URL_KEY {
            Value::String(
                self.impl_.cached_resource_options.lock().unwrap().tile_server_options().base_url().to_owned(),
            )
        } else if key == MAX_CONCURRENT_REQUESTS_KEY {
            Value::Uint(u64::from(*self.impl_.cached_maximum_concurrent_requests.lock().unwrap()))
        } else {
            Log::error(Event::General, format!("Resource provider does not support property {key}"));
            Value::Null
        }
    }

    fn set_resource_options(&self, options: ResourceOptions) {
        let o = options.clone();
        self.impl_.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.set_resource_options(o));
        *self.impl_.cached_resource_options.lock().unwrap() = options;
    }

    fn get_resource_options(&self) -> ResourceOptions {
        self.impl_.cached_resource_options.lock().unwrap().clone()
    }

    fn set_client_options(&self, options: ClientOptions) {
        let o = options.clone();
        self.impl_.thread.actor().invoke(move |t: &OnlineFileSourceThread| t.set_client_options(o));
        *self.impl_.cached_client_options.lock().unwrap() = options;
    }

    fn get_client_options(&self) -> ClientOptions {
        self.impl_.cached_client_options.lock().unwrap().clone()
    }
}

/// The HTTP transfers that carry the headers of the runtime's HTTP header
/// transform: the core's HTTPFileSource transfers (ureq on worker threads,
/// the response read as curl's header callback and `handleResult` do), with
/// the added headers, following redirects here rather than in ureq, so that a
/// redirect to another origin drops the added headers.
#[cfg(not(target_os = "emscripten"))]
mod header_transport {
    use maplibre_native_next::mln_actor::{ActorRef, Mailbox, scheduler};
    use maplibre_native_next::mln_storage::async_request::AsyncRequest;
    use maplibre_native_next::mln_storage::file_source::Callback;
    use maplibre_native_next::mln_storage::resource::{Kind, Resource};
    use maplibre_native_next::mln_storage::response::{Error, Reason, Response};
    use maplibre_native_next::mln_util::chrono::{parse_timestamp_str, rfc1123};
    use maplibre_native_next::mln_util::constants::DEFAULT_MAXIMUM_CONCURRENT_REQUESTS;
    use maplibre_native_next::mln_util::http_header::{CacheControl, parse_retry_headers};
    use std::any::Any;
    use std::cell::RefCell;
    use std::rc::Rc;
    use std::sync::atomic::{AtomicBool, Ordering};
    use std::sync::{Arc, Mutex, mpsc};

    /// curl's default for CURLOPT_MAXREDIRS is unlimited; ureq's is 10.
    const MAX_REDIRECTS: u32 = 10;

    struct RequestState {
        callback: RefCell<Callback>,
    }

    struct HeaderRequest {
        mailbox: Arc<Mailbox>,
        cancelled: Arc<AtomicBool>,
    }

    impl AsyncRequest for HeaderRequest {}

    impl Drop for HeaderRequest {
        fn drop(&mut self) {
            self.cancelled.store(true, Ordering::SeqCst);
            self.mailbox.close();
        }
    }

    struct Job {
        resource: Resource,
        headers: Vec<(String, String)>,
        reply: ActorRef<RequestState>,
        cancelled: Arc<AtomicBool>,
    }

    pub struct HeaderHTTPFileSource {
        jobs: Mutex<mpsc::Sender<Job>>,
    }

    impl HeaderHTTPFileSource {
        pub fn new() -> Self {
            let agent: ureq::Agent = ureq::Agent::config_builder()
                .http_status_as_error(false)
                .user_agent("MapLibreNative/1.0")
                .max_redirects(0)
                .build()
                .into();
            let (sender, receiver) = mpsc::channel::<Job>();
            let receiver = Arc::new(Mutex::new(receiver));
            for index in 0..DEFAULT_MAXIMUM_CONCURRENT_REQUESTS {
                let agent = agent.clone();
                let receiver = receiver.clone();
                // Detached: a worker exits when the channel closes, after its
                // current transfer.
                let _ = std::thread::Builder::new()
                    .name(format!("HTTPFileSource headers {index}"))
                    .spawn(move || worker(agent, receiver));
            }
            HeaderHTTPFileSource { jobs: Mutex::new(sender) }
        }

        /// `HTTPFileSource::request` with `headers`. The callback runs on the
        /// calling thread's RunLoop.
        pub fn request(
            &self,
            resource: &Resource,
            headers: Vec<(String, String)>,
            callback: Callback,
        ) -> Box<dyn AsyncRequest> {
            let state = Rc::new(RequestState { callback: RefCell::new(callback) });
            let mailbox = Mailbox::with_target(&scheduler::get_current(), state as Rc<dyn Any>);
            let cancelled = Arc::new(AtomicBool::new(false));
            let job = Job {
                resource: resource.clone(),
                headers,
                reply: ActorRef::new(&mailbox),
                cancelled: cancelled.clone(),
            };
            let _ = self.jobs.lock().unwrap().send(job);
            Box::new(HeaderRequest { mailbox, cancelled })
        }
    }

    fn worker(agent: ureq::Agent, jobs: Arc<Mutex<mpsc::Receiver<Job>>>) {
        loop {
            let job = match jobs.lock() {
                Ok(receiver) => receiver.recv(),
                Err(_) => return,
            };
            let Ok(job) = job else {
                return; // the file source is gone
            };
            if job.cancelled.load(Ordering::SeqCst) {
                continue;
            }
            let response = perform(&agent, &job.resource, &job.headers);
            if !job.cancelled.load(Ordering::SeqCst) {
                job.reply.invoke(move |request: &RequestState| (request.callback.borrow_mut())(response));
            }
        }
    }

    /// A URL's origin: its scheme, host and port, compared without case.
    fn origin(url: &str) -> Option<(String, String, u16)> {
        let uri: ureq::http::Uri = url.parse().ok()?;
        let scheme = uri.scheme_str()?.to_ascii_lowercase();
        let host = uri.host()?.to_ascii_lowercase();
        let port = uri.port_u16().unwrap_or(if scheme == "https" { 443 } else { 80 });
        Some((scheme, host, port))
    }

    /// The URL that a redirect's `Location` names, relative to `base`.
    fn resolve(base: &str, location: &str) -> Option<String> {
        if location.contains("://") {
            return Some(location.to_owned());
        }
        let uri: ureq::http::Uri = base.parse().ok()?;
        let scheme = uri.scheme_str()?;
        if let Some(rest) = location.strip_prefix("//") {
            return Some(format!("{scheme}://{rest}"));
        }
        let authority = uri.authority()?.as_str();
        if location.starts_with('/') {
            return Some(format!("{scheme}://{authority}{location}"));
        }
        let path = uri.path();
        let directory = &path[..path.rfind('/').map_or(0, |slash| slash + 1)];
        let directory = if directory.is_empty() { "/" } else { directory };
        Some(format!("{scheme}://{authority}{directory}{location}"))
    }

    fn connection_error(error: &ureq::Error) -> bool {
        matches!(
            error,
            ureq::Error::HostNotFound | ureq::Error::ConnectionFailed | ureq::Error::Timeout(_) | ureq::Error::Io(_)
        )
    }

    fn error(error: ureq::Error) -> Box<Error> {
        let reason = if connection_error(&error) { Reason::Connection } else { Reason::Other };
        Box::new(Error::new(reason, error.to_string()))
    }

    struct Transfer {
        status: i64,
        headers: Vec<(String, String)>,
        body: Result<Vec<u8>, Box<Error>>,
    }

    fn transfer(agent: &ureq::Agent, resource: &Resource, added: &[(String, String)]) -> Result<Transfer, Box<Error>> {
        let first_origin = origin(&resource.url);
        let mut url = resource.url.clone();
        let mut redirects = 0;
        loop {
            let mut request = agent.get(&url);
            if let Some((first, second)) = resource.data_range {
                request = request.header("Range", format!("bytes={first}-{second}"));
            }
            // If there's already a response, set the correct etags/modified
            // headers to make sure we are getting a 304 response if possible.
            if let Some(prior_etag) = &resource.prior_etag {
                request = request.header("If-None-Match", prior_etag.as_str());
            } else if let Some(prior_modified) = resource.prior_modified {
                request = request.header("If-Modified-Since", rfc1123(prior_modified));
            }
            if first_origin.is_some() && origin(&url) == first_origin {
                for (name, value) in added {
                    request = request.header(name.as_str(), value.as_str());
                }
            }
            let mut response = request.call().map_err(error)?;
            let status = response.status().as_u16();
            let location =
                response.headers().get("location").and_then(|location| location.to_str().ok()).map(str::to_owned);
            if matches!(status, 301 | 302 | 303 | 307 | 308)
                && let Some(location) = location
                && redirects < MAX_REDIRECTS
                && let Some(next) = resolve(&url, &location)
            {
                redirects += 1;
                url = next;
                continue;
            }
            let headers = response
                .headers()
                .iter()
                .filter_map(|(name, value)| Some((name.as_str().to_owned(), value.to_str().ok()?.to_owned())))
                .collect();
            let status = i64::from(status);
            let body = if status == 200 || status == 206 {
                response.body_mut().with_config().limit(u64::MAX).read_to_vec().map_err(error)
            } else {
                Ok(Vec::new())
            };
            return Ok(Transfer { status, headers, body });
        }
    }

    /// The core's `perform`: the response that a transfer makes.
    fn perform(agent: &ureq::Agent, resource: &Resource, headers: &[(String, String)]) -> Response {
        let mut response = Response::new();
        let transfer = match transfer(agent, resource, headers) {
            Ok(transfer) => transfer,
            Err(error) => {
                response.error = Some(error);
                return response;
            }
        };
        let mut retry_after: Option<String> = None;
        let mut x_rate_limit_reset: Option<String> = None;
        for (name, value) in &transfer.headers {
            if name.eq_ignore_ascii_case("last-modified") {
                response.modified = Some(parse_timestamp_str(value));
            } else if name.eq_ignore_ascii_case("etag") {
                response.etag = Some(value.clone());
            } else if name.eq_ignore_ascii_case("cache-control") {
                let cache_control = CacheControl::parse(value);
                response.expires = cache_control.to_time_point();
                response.must_revalidate = cache_control.must_revalidate;
            } else if name.eq_ignore_ascii_case("expires") {
                response.expires = Some(parse_timestamp_str(value));
            } else if name.eq_ignore_ascii_case("retry-after") {
                retry_after = Some(value.clone());
            } else if name.eq_ignore_ascii_case("x-rate-limit-reset") {
                x_rate_limit_reset = Some(value.clone());
            }
        }
        let code = transfer.status;
        if code == 200 || code == 206 {
            match transfer.body {
                Ok(data) => response.data = Some(Arc::new(data)),
                Err(error) => response.error = Some(error),
            }
        } else if code == 204 || (code == 404 && resource.kind == Kind::Tile) {
            response.no_content = true;
        } else if code == 304 {
            response.not_modified = true;
        } else if code == 404 {
            response.error = Some(Box::new(Error::new(Reason::NotFound, "HTTP status code 404")));
        } else if code == 429 {
            response.error = Some(Box::new(Error::with_retry_after(
                Reason::RateLimit,
                "HTTP status code 429",
                parse_retry_headers(&retry_after, &x_rate_limit_reset),
            )));
        } else if (500..600).contains(&code) {
            response.error = Some(Box::new(Error::new(Reason::Server, format!("HTTP status code {code}"))));
        } else {
            response.error = Some(Box::new(Error::new(Reason::Other, format!("HTTP status code {code}"))));
        }
        response
    }

    #[cfg(test)]
    mod tests {
        use super::{origin, resolve};

        #[test]
        fn locations_resolve_against_the_request() {
            assert_eq!(resolve("http://a:1/x/y.json", "/z.json").as_deref(), Some("http://a:1/z.json"));
            assert_eq!(resolve("http://a:1/x/y.json", "z.json").as_deref(), Some("http://a:1/x/z.json"));
            assert_eq!(resolve("http://a:1/x/y.json", "//b/z").as_deref(), Some("http://b/z"));
            assert_eq!(resolve("http://a:1/x", "https://c/d").as_deref(), Some("https://c/d"));
            assert_eq!(origin("HTTP://Example.com/x"), origin("http://example.com:80/y"));
            assert_ne!(origin("http://127.0.0.1:1/x"), origin("http://127.0.0.1:2/x"));
        }
    }
}

/// The browser's transport can't drop added headers on a redirect, so the
/// runtime refuses an HTTP header transform there and nothing builds this.
#[cfg(target_os = "emscripten")]
mod header_transport {
    use maplibre_native_next::mln_storage::async_request::AsyncRequest;
    use maplibre_native_next::mln_storage::file_source::Callback;
    use maplibre_native_next::mln_storage::resource::Resource;

    pub struct HeaderHTTPFileSource;

    impl HeaderHTTPFileSource {
        pub fn new() -> Self {
            HeaderHTTPFileSource
        }

        pub fn request(&self, _: &Resource, _: Vec<(String, String)>, _: Callback) -> Box<dyn AsyncRequest> {
            unreachable!("the browser has no HTTP header transform")
        }
    }
}
