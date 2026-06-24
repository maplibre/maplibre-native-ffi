#[cfg(not(target_os = "emscripten"))]
mod http_client;
mod image;

#[cfg(target_os = "android")]
mod android;
