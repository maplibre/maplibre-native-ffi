// Ports MapLibre Native's platform/default/src/mbgl/storage/
// database_file_source.cpp, the database file source that the C API's
// MapLibre build registers (src/resources/file_source_manager.cpp,
// make_database_file_source), onto maplibre-native-next's OfflineDatabase and
// OfflineDownload. The core's DatabaseFileSource has no offline region API
// yet, so the runtime's `Database` file source is this one.
//
// The database lives on its own thread ("DatabaseFileSource"), with the
// region downloads, which load through the runtime's network file source.
// Requests reach it as messages and answer through the requesting thread's
// FileSourceRequest; the offline operations run there through `run`.

use maplibre_native_next::maplibre_native_base::Value;
use maplibre_native_next::mln_actor::{ActorRef, Thread, scheduler};
use maplibre_native_next::mln_storage::file_source::{Callback, READ_ONLY_MODE_KEY};
use maplibre_native_next::mln_storage::file_source_request::FileSourceRequestState;
use maplibre_native_next::mln_storage::offline::{
    OfflineRegionDefinition, OfflineRegionDownloadState, OfflineRegionObserver, OfflineRegionStatus,
};
use maplibre_native_next::mln_storage::offline_database::DatabaseError;
use maplibre_native_next::mln_storage::resource::StoragePolicy;
use maplibre_native_next::mln_storage::response::{Error, Reason};
use maplibre_native_next::mln_storage::{
    AsyncRequest, ClientOptions, FileSource, FileSourceManager, FileSourceRequest, FileSourceType, LoadingMethod,
    OfflineDatabase, Resource, ResourceOptions, Response,
};
use maplibre_native_next::mln_storage_offline::OfflineDownload;
use maplibre_native_next::mln_util::constants::{ASSET_PROTOCOL, FILE_PROTOCOL};
use maplibre_native_next::mln_util::logging::{Event, Log};
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;
use std::sync::{Arc, Mutex};

/// `DatabaseFileSourceThread`: the database and the region downloads, on
/// the database thread.
pub struct DatabaseThread {
    pub db: Rc<RefCell<OfflineDatabase>>,
    downloads: RefCell<HashMap<i64, Rc<OfflineDownload>>>,
    online_file_source: Option<Arc<dyn FileSource>>,
}

impl DatabaseThread {
    /// `request(const Resource&, const ActorRef<FileSourceRequest>&)`.
    fn request(&self, resource: &Resource, req: &ActorRef<FileSourceRequestState>) {
        let offline_response =
            if resource.storage_policy != StoragePolicy::Volatile { self.db.borrow_mut().get(resource) } else { None };
        let response = match offline_response {
            None => {
                let mut response = Response::new();
                response.no_content = true;
                response.error = Some(Box::new(Error::new(Reason::NotFound, "Not found in offline database")));
                response
            }
            Some(mut response) => {
                if !response.is_usable() {
                    response.error = Some(Box::new(Error::new(Reason::NotFound, "Cached resource is unusable")));
                }
                response
            }
        };
        req.invoke(move |request| request.set_response(response));
    }

    /// `getDownload(int64_t regionID)`: the region's download, which the
    /// thread keeps from its first use until the region is deleted.
    pub fn download(&self, region_id: i64) -> Result<Rc<OfflineDownload>, DatabaseError> {
        let Some(online_file_source) = &self.online_file_source else {
            return Err(DatabaseError::Runtime("Network file source unavailable.".to_owned()));
        };
        if let Some(download) = self.downloads.borrow().get(&region_id) {
            return Ok(download.clone());
        }
        let definition: OfflineRegionDefinition = self.db.borrow_mut().get_region_definition(region_id)?;
        let download =
            Rc::new(OfflineDownload::new(region_id, definition, self.db.clone(), online_file_source.clone()));
        self.downloads.borrow_mut().insert(region_id, download.clone());
        Ok(download)
    }

    /// `getRegionStatus(int64_t regionID, callback)`.
    pub fn region_status(&self, region_id: i64) -> Result<OfflineRegionStatus, DatabaseError> {
        Ok(self.download(region_id)?.get_status())
    }

    /// `setRegionObserver(int64_t regionID, observer)`.
    pub fn set_region_observer(&self, region_id: i64, observer: Option<Rc<dyn OfflineRegionObserver>>) {
        if let Ok(download) = self.download(region_id) {
            download.set_observer(observer);
        }
    }

    /// `setRegionDownloadState(int64_t regionID, OfflineRegionDownloadState)`.
    pub fn set_region_download_state(&self, region_id: i64, state: OfflineRegionDownloadState) {
        if let Ok(download) = self.download(region_id) {
            download.set_state(state);
        }
    }

    /// The `downloads.erase(region.getID())` of `deleteRegion`.
    pub fn forget_download(&self, region_id: i64) {
        let download = self.downloads.borrow_mut().remove(&region_id);
        drop(download);
    }
}

impl Drop for DatabaseThread {
    fn drop(&mut self) {
        // The downloads hold the database; they stop before it closes.
        let downloads = std::mem::take(&mut *self.downloads.borrow_mut());
        drop(downloads);
    }
}

/// `DatabaseFileSource`.
pub struct DatabaseFileSource {
    thread: Thread<DatabaseThread>,
    resource_options: Mutex<ResourceOptions>,
    client_options: Mutex<ClientOptions>,
}

impl DatabaseFileSource {
    pub fn new(resource_options: &ResourceOptions, client_options: &ClientOptions) -> Self {
        let online_file_source =
            FileSourceManager::get().get_file_source(FileSourceType::Network, resource_options, client_options);
        let tile_server_options = match &online_file_source {
            Some(online) => online.get_resource_options().tile_server_options(),
            None => resource_options.tile_server_options(),
        };
        let cache_path = resource_options.cache_path().to_owned();
        let thread = Thread::new("DatabaseFileSource", move |_| DatabaseThread {
            db: Rc::new(RefCell::new(OfflineDatabase::new(cache_path, &tile_server_options))),
            downloads: RefCell::new(HashMap::new()),
            online_file_source,
        });
        DatabaseFileSource {
            thread,
            resource_options: Mutex::new(resource_options.clone()),
            client_options: Mutex::new(client_options.clone()),
        }
    }

    /// Runs `task` on the database thread, after the work queued before it.
    pub fn run(&self, task: impl FnOnce(&DatabaseThread) + Send + 'static) {
        self.thread.actor().invoke(task);
    }
}

impl FileSource for DatabaseFileSource {
    fn request(&self, resource: &Resource, callback: Callback) -> Box<dyn AsyncRequest> {
        let request = FileSourceRequest::new(callback);
        let reply = request.actor();
        let resource = resource.clone();
        self.run(move |thread| thread.request(&resource, &reply));
        Box::new(request)
    }

    fn forward(&self, resource: &Resource, response: &Response, callback: Option<Box<dyn FnOnce() + Send>>) {
        if resource.storage_policy == StoragePolicy::Volatile {
            return;
        }
        let current = scheduler::get_current();
        let mut callback = callback.map(|callback| scheduler::bind_once(&current, callback));
        let resource = resource.clone();
        let response = response.clone();
        self.run(move |thread| {
            thread.db.borrow_mut().put(&resource, &response);
            if let Some(callback) = callback.as_mut() {
                callback();
            }
        });
    }

    fn can_request(&self, resource: &Resource) -> bool {
        resource.has_loading_method(LoadingMethod::Cache)
            && !resource.url.starts_with(ASSET_PROTOCOL)
            && !resource.url.starts_with(FILE_PROTOCOL)
    }

    fn pause(&self) {
        self.thread.pause();
    }

    fn resume(&self) {
        self.thread.resume();
    }

    fn set_property(&self, key: &str, value: &Value) {
        match value {
            Value::Bool(read_only) if key == READ_ONLY_MODE_KEY => {
                let read_only = *read_only;
                self.run(move |thread| thread.db.borrow_mut().reopen_database_read_only(read_only));
            }
            _ => Log::error(Event::General, format!("Resource provider does not support property {key}")),
        }
    }

    fn set_resource_options(&self, options: ResourceOptions) {
        *self.resource_options.lock().unwrap() = options;
    }

    fn get_resource_options(&self) -> ResourceOptions {
        self.resource_options.lock().unwrap().clone()
    }

    fn set_client_options(&self, options: ClientOptions) {
        *self.client_options.lock().unwrap() = options;
    }

    fn get_client_options(&self) -> ClientOptions {
        self.client_options.lock().unwrap().clone()
    }

    fn as_any(&self) -> Option<&dyn std::any::Any> {
        Some(self)
    }
}
