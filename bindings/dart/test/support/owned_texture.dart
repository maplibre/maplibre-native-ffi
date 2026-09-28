import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

import 'vulkan_context.dart';
import 'wgl_context.dart';

abstract class OwnedTextureContext {
  static OwnedTextureContext create() {
    final backends = supportedRenderBackendMask();
    if (backends.contains(RenderBackendFlag.metal)) return _MetalContext();
    if (backends.contains(RenderBackendFlag.vulkan)) return _VulkanContext();
    if (backends.contains(RenderBackendFlag.opengl) &&
        openglSupportedContextProviderMask().contains(
          OpenglContextProviderFlag.egl,
        )) {
      return _EglContext();
    }
    if (backends.contains(RenderBackendFlag.opengl) &&
        openglSupportedContextProviderMask().contains(
          OpenglContextProviderFlag.wgl,
        )) {
      return _WglContext();
    }
    throw StateError('No owned texture fixture for backend $backends');
  }

  RenderSessionAttachment attach(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  );
  void close();
}

final class _MetalContext extends OwnedTextureContext {
  _MetalContext() {
    device =
        DynamicLibrary.open(
          '/System/Library/Frameworks/Metal.framework/Metal',
        ).lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
          'MTLCreateSystemDefaultDevice',
        )();
    if (device == nullptr) throw StateError('Metal returned no device');
  }
  late final Pointer<Void> device;
  @override
  RenderSessionAttachment attach(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  ) => map.metalOwnedTextureAttach(
    MetalOwnedTextureDescriptor(
      extent: extent,
      context: MetalContextDescriptor(device: NativePointer(device.address)),
    ),
    options,
  );
  @override
  void close() =>
      DynamicLibrary.open('/usr/lib/libobjc.A.dylib').lookupFunction<
        Void Function(Pointer<Void>),
        void Function(Pointer<Void>)
      >('objc_release')(device);
}

final class _VulkanContext extends OwnedTextureContext {
  final context = VulkanTestContext();
  @override
  RenderSessionAttachment attach(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  ) => map.vulkanOwnedTextureAttach(
    VulkanOwnedTextureDescriptor(extent: extent, context: context.descriptor),
    options,
  );
  @override
  void close() => context.close();
}

final class _EglContext extends OwnedTextureContext {
  _EglContext() {
    library = DynamicLibrary.open(
      Platform.isMacOS
          ? '${File('.dart_tool/maplibre_native_install_dir').readAsStringSync().trim()}/lib/libEGL.dylib'
          : Platform.isWindows
          ? 'libEGL.dll'
          : 'libEGL.so.1',
    );
    using((arena) {
      final getPlatform = library
          .lookupFunction<
            Pointer<Void> Function(Uint32, Pointer<Void>, Pointer<IntPtr>),
            Pointer<Void> Function(int, Pointer<Void>, Pointer<IntPtr>)
          >('eglGetPlatformDisplay');
      final initialize = library
          .lookupFunction<
            Uint32 Function(Pointer<Void>, Pointer<Int32>, Pointer<Int32>),
            int Function(Pointer<Void>, Pointer<Int32>, Pointer<Int32>)
          >('eglInitialize');
      if (Platform.isMacOS) {
        final attributes = arena<IntPtr>(5);
        const values = [0x3203, 0x3489, 0x3209, 0x320A, 0x3038];
        for (var i = 0; i < values.length; i++) {
          attributes[i] = values[i];
        }
        display = getPlatform(0x3202, nullptr, attributes);
      } else {
        display = getPlatform(0x31DD, nullptr, nullptr);
      }
      if (display == nullptr || initialize(display, nullptr, nullptr) == 0) {
        display = library
            .lookupFunction<
              Pointer<Void> Function(Pointer<Void>),
              Pointer<Void> Function(Pointer<Void>)
            >('eglGetDisplay')(nullptr);
        if (display == nullptr || initialize(display, nullptr, nullptr) == 0) {
          throw StateError('EGL could not initialize a display');
        }
      }
      try {
        if (library.lookupFunction<Uint32 Function(Uint32), int Function(int)>(
              'eglBindAPI',
            )(0x30A0) ==
            0) {
          throw StateError('EGL rejected OpenGL ES');
        }
        final values = [
          0x3033,
          1,
          0x3040,
          0x40,
          0x3024,
          8,
          0x3023,
          8,
          0x3022,
          8,
          0x3021,
          8,
          0x3025,
          24,
          0x3026,
          8,
          0x3038,
        ];
        final attributes = arena<Int32>(values.length)
          ..asTypedList(values.length).setAll(0, values);
        final chosen = arena<Pointer<Void>>();
        final count = arena<Int32>();
        final success = library
            .lookupFunction<
              Uint32 Function(
                Pointer<Void>,
                Pointer<Int32>,
                Pointer<Pointer<Void>>,
                Int32,
                Pointer<Int32>,
              ),
              int Function(
                Pointer<Void>,
                Pointer<Int32>,
                Pointer<Pointer<Void>>,
                int,
                Pointer<Int32>,
              )
            >('eglChooseConfig')(display, attributes, chosen, 1, count);
        if (success == 0 || count.value == 0) {
          throw StateError('EGL has no ES 3 pbuffer config');
        }
        config = chosen.value;
      } catch (_) {
        close();
        rethrow;
      }
    });
  }
  late final DynamicLibrary library;
  Pointer<Void> display = nullptr, config = nullptr, shared = nullptr;
  @override
  RenderSessionAttachment attach(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  ) {
    final caller = options.driver == RenderDriverKind.callerGraphicsThread;
    if (caller && shared == nullptr) {
      shared = using((arena) {
        final attributes = arena<Int32>(3);
        attributes.asTypedList(3).setAll(0, [0x3098, 3, 0x3038]);
        return library.lookupFunction<
          Pointer<Void> Function(
            Pointer<Void>,
            Pointer<Void>,
            Pointer<Void>,
            Pointer<Int32>,
          ),
          Pointer<Void> Function(
            Pointer<Void>,
            Pointer<Void>,
            Pointer<Void>,
            Pointer<Int32>,
          )
        >('eglCreateContext')(display, config, nullptr, attributes);
      });
      if (shared == nullptr) {
        throw StateError('EGL could not create a share context');
      }
    }
    return map.openglOwnedTextureAttach(
      OpenglOwnedTextureDescriptor(
        extent: extent,
        context: OpenglContextDescriptor(
          ownership: caller
              ? OpenglContextOwnership.shared
              : OpenglContextOwnership.dedicated,
          data: OpenglContextDescriptorDataEgl(
            EglContextDescriptor(
              display: NativePointer(display.address),
              config: NativePointer(config.address),
              shareContext: NativePointer(caller ? shared.address : 0),
              clientApi: OpenglClientApi.gles,
            ),
          ),
        ),
      ),
      options,
    );
  }

  @override
  void close() {
    if (display != nullptr) {
      if (shared != nullptr) {
        library.lookupFunction<
          Uint32 Function(Pointer<Void>, Pointer<Void>),
          int Function(Pointer<Void>, Pointer<Void>)
        >('eglDestroyContext')(display, shared);
        shared = nullptr;
      }
      library.lookupFunction<
        Uint32 Function(Pointer<Void>),
        int Function(Pointer<Void>)
      >('eglTerminate')(display);
      display = nullptr;
    }
  }
}

final class _WglContext extends OwnedTextureContext {
  final context = WglTestContext();
  @override
  RenderSessionAttachment attach(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  ) {
    final caller = options.driver == RenderDriverKind.callerGraphicsThread;
    return map.openglOwnedTextureAttach(
      OpenglOwnedTextureDescriptor(
        extent: extent,
        context: OpenglContextDescriptor(
          ownership: caller
              ? OpenglContextOwnership.shared
              : OpenglContextOwnership.dedicated,
          data: OpenglContextDescriptorDataWgl(
            context.descriptor(shared: caller),
          ),
        ),
      ),
      options,
    );
  }

  @override
  void close() => context.close();
}
