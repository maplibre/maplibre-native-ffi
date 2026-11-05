# The GObject library over a native install's maplibre-native-c. It installs
# into that prefix, beside the C library: the shared library, its headers and
# pkg-config file, the introspection data, and the Vala bindings.
project(
  'maplibre-native-ffi-gobject',
  'c',
  # The release stamps its version into the package it publishes.
  version: '0.0.0',
  meson_version: '>= 1.3.0',
  # The C headers declare enums with a fixed underlying type, which C23 added.
  # Ubuntu 24.04's GCC 13 and meson 1.3 know C23 only as c2x. The pkg-config
  # file names its prefix relative to itself, as the C library's does, so the
  # prefix can move.
  default_options: [
    'c_std=c2x',
    'warning_level=2',
    'werror=true',
    'pkgconfig.relocatable=true',
  ],
)

api_version = '0'
namespace = 'MaplibreNativeFfi'
package = 'maplibre-native-ffi-gobject-' + api_version

# Ubuntu 24.04's versions are the oldest the library supports.
glib_version = '>= 2.80'
gio = dependency('gio-2.0', version: glib_version)
native = dependency('maplibre-native-c')
add_project_arguments(
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_80',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_80',
  language: 'c',
)

headers = files(
  'src/maplibre-native-ffi-enums.h',
  'src/maplibre-native-ffi-handles.h',
  'src/maplibre-native-ffi-values.h',
  'src/maplibre-native-ffi.h',
)
library = shared_library(
  package,
  'src/maplibre-native-ffi-dispatch.c',
  'src/maplibre-native-ffi-enums.c',
  'src/maplibre-native-ffi-handles.c',
  'src/maplibre-native-ffi-values.c',
  dependencies: [gio, native],
  soversion: api_version,
  install: true,
)
install_headers(headers, subdir: package)

# The C library's pkg-config file lives in the prefix's share directory, so
# this one does too, and one search path finds both. Like that file, it gives
# programs an rpath to the prefix.
import('pkgconfig').generate(
  library,
  libraries: ['-Wl,-rpath,${libdir}'],
  name: package,
  description: 'GObject bindings for MapLibre Native',
  requires: [gio],
  requires_private: [native],
  subdirs: package,
  install_dir: get_option('datadir') / 'pkgconfig',
)

gnome = import('gnome')
gir = gnome.generate_gir(
  library,
  sources: headers,
  namespace: namespace,
  nsversion: api_version,
  identifier_prefix: namespace,
  symbol_prefix: 'maplibre_native_ffi',
  includes: ['GObject-2.0', 'Gio-2.0'],
  header: 'maplibre-native-ffi.h',
  export_packages: package,
  extra_args: ['--warn-all'],
  fatal_warnings: true,
  install: true,
)
gnome.generate_vapi(
  package,
  sources: gir[0],
  packages: ['gio-2.0'],
  install: true,
)
