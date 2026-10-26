// The browser takes its HTTP from emscripten_fetch and Android from
// HttpURLConnection, so neither builds the HTTP client. Image decoding is shared
// by every platform, and only Linux, FreeBSD, and Windows take their locale
// services from here.
#[cfg(not(any(target_os = "emscripten", target_os = "android")))]
mod http_client;
#[cfg(any(target_os = "linux", target_os = "freebsd", windows))]
mod i18n;
mod image;
