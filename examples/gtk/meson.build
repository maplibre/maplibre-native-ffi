# The GTK example. It builds against the GObject library in a native preset's
# install, through that prefix's pkg-config files and Vala bindings, as an app
# outside this repository builds against an installed one.
project(
  'maplibre-native-ffi-gtk-example',
  'vala',
  'c',
  version: '0.0.0',
  meson_version: '>= 1.3.0',
)

# Ubuntu 24.04's versions are the oldest the example supports. GTK 4.14 added
# GdkDmabufTextureBuilder and GtkGraphicsOffload.
gtk = dependency('gtk4', version: '>= 4.14')
maplibre = dependency('maplibre-native-ffi-gobject-0')
posix = meson.get_compiler('vala').find_library('posix')
math = meson.get_compiler('c').find_library('m', required: false)

add_project_arguments(
  '--target-glib=2.80',
  '--vapidir',
  maplibre.get_variable(pkgconfig: 'prefix') / 'share' / 'vala' / 'vapi',
  language: 'vala',
)
add_project_arguments(
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_80',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_80',
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_14',
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_14',
  language: 'c',
)

# The C that valac writes warns about variables that it sets but doesn't
# use, so only the handwritten C is held to warnings.
dma_buf = static_library(
  'dma-buf',
  'src/dma-buf.c',
  override_options: ['warning_level=3', 'werror=true'],
)
executable(
  'maplibre-gtk-example',
  'src/gestures.vala',
  'src/main.vala',
  'src/map-view.vala',
  dependencies: [gtk, maplibre, posix, math],
  include_directories: 'src',
  link_with: dma_buf,
  override_options: ['warning_level=0'],
)
