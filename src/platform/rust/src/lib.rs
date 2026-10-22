// The browser takes its HTTP from emscripten_fetch and has no JNI, so neither
// module is built there. Image decoding is shared by every platform, and only
// Linux takes its locale services from here.
#[cfg(not(target_os = "emscripten"))]
mod android;
#[cfg(not(target_os = "emscripten"))]
mod http_client;
#[cfg(target_os = "linux")]
mod i18n;
mod image;
