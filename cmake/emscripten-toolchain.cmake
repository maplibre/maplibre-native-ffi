if(NOT DEFINED ENV{EMSDK})
  message(FATAL_ERROR "EMSDK is required to configure the Emscripten toolchain")
endif()

if(DEFINED ENV{EMSDK_NODE})
  set(CMAKE_CROSSCOMPILING_EMULATOR "$ENV{EMSDK_NODE}"
      CACHE FILEPATH "Node.js executable used to run Emscripten outputs")
endif()

include(
        "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
