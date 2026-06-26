from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import glfw
import vulkan as vk

from maplibre_native import render


class VulkanUnavailableError(RuntimeError):
    pass


def _addr(value: Any) -> int:
    return int(vk.ffi.cast("uintptr_t", value))


def _pointer(value: Any, name: str) -> render.NativePointer:
    return render.NativePointer(_addr(value), _diagnostic_name=name)


def _extension_name(extension: Any) -> str:
    raw = extension.extensionName
    if isinstance(raw, str):
        return raw.split("\0", 1)[0]
    return bytes(raw).split(b"\0", 1)[0].decode()


def _has_device_extension(physical_device: Any, name: str) -> bool:
    return any(
        _extension_name(extension) == name
        for extension in vk.vkEnumerateDeviceExtensionProperties(physical_device, None)
    )


def _find_graphics_queue_family(physical_device: Any) -> int | None:
    for index, family in enumerate(
        vk.vkGetPhysicalDeviceQueueFamilyProperties(physical_device)
    ):
        if family.queueCount > 0 and family.queueFlags & vk.VK_QUEUE_GRAPHICS_BIT:
            return index
    return None


def _find_memory_type(
    physical_device: Any,
    type_filter: int,
    properties: int,
) -> int:
    memory = vk.vkGetPhysicalDeviceMemoryProperties(physical_device)
    for index in range(memory.memoryTypeCount):
        if not (type_filter & (1 << index)):
            continue
        if memory.memoryTypes[index].propertyFlags & properties == properties:
            return index
    msg = "no compatible Vulkan memory type found"
    raise VulkanUnavailableError(msg)


def _device_features(physical_device: Any) -> Any:
    supported = vk.vkGetPhysicalDeviceFeatures(physical_device)
    features = vk.VkPhysicalDeviceFeatures()
    features.samplerAnisotropy = supported.samplerAnisotropy
    features.wideLines = supported.wideLines
    return features


@dataclass(slots=True)
class VulkanContext:
    instance: Any
    physical_device: Any
    device: Any
    queue: Any
    queue_family_index: int
    _glfw_initialized: bool = False
    _closed: bool = False

    @classmethod
    def create(cls, *, surface_extensions: bool = False) -> "VulkanContext":
        if surface_extensions:
            if not glfw.init():
                msg = "GLFW could not initialize for Vulkan surface tests"
                raise VulkanUnavailableError(msg)
            extensions = glfw.get_required_instance_extensions()
            if not extensions:
                glfw.terminate()
                msg = "GLFW did not report Vulkan surface instance extensions"
                raise VulkanUnavailableError(msg)
        else:
            extensions = []

        app = vk.VkApplicationInfo(
            pApplicationName="maplibre-native-python-render-tests",
            applicationVersion=1,
            pEngineName="maplibre-native-ffi",
            engineVersion=1,
            apiVersion=vk.VK_API_VERSION_1_0,
        )
        instance_info = vk.VkInstanceCreateInfo(
            pApplicationInfo=app,
            enabledExtensionCount=len(extensions),
            ppEnabledExtensionNames=extensions,
        )
        try:
            instance = vk.vkCreateInstance(instance_info, None)
        except Exception as error:  # pragma: no cover - depends on host Vulkan ICD
            if surface_extensions:
                glfw.terminate()
            msg = f"Vulkan instance creation failed: {error}"
            raise VulkanUnavailableError(msg) from error

        device = None
        try:
            for physical_device in vk.vkEnumeratePhysicalDevices(instance):
                queue_family_index = _find_graphics_queue_family(physical_device)
                if queue_family_index is None:
                    continue

                enabled_extensions: list[str] = []
                if surface_extensions:
                    if not _has_device_extension(
                        physical_device,
                        "VK_KHR_swapchain",
                    ):
                        continue
                    enabled_extensions.append("VK_KHR_swapchain")
                if _has_device_extension(physical_device, "VK_KHR_portability_subset"):
                    enabled_extensions.append("VK_KHR_portability_subset")

                queue = vk.VkDeviceQueueCreateInfo(
                    queueFamilyIndex=queue_family_index,
                    queueCount=1,
                    pQueuePriorities=[1.0],
                )
                device_info = vk.VkDeviceCreateInfo(
                    queueCreateInfoCount=1,
                    pQueueCreateInfos=[queue],
                    enabledExtensionCount=len(enabled_extensions),
                    ppEnabledExtensionNames=enabled_extensions,
                    pEnabledFeatures=_device_features(physical_device),
                )
                try:
                    device = vk.vkCreateDevice(physical_device, device_info, None)
                except Exception:
                    continue

                graphics_queue = vk.vkGetDeviceQueue(device, queue_family_index, 0)
                return cls(
                    instance=instance,
                    physical_device=physical_device,
                    device=device,
                    queue=graphics_queue,
                    queue_family_index=queue_family_index,
                    _glfw_initialized=surface_extensions,
                )
        except Exception:
            if device is not None:
                vk.vkDestroyDevice(device, None)
            vk.vkDestroyInstance(instance, None)
            if surface_extensions:
                glfw.terminate()
            raise

        vk.vkDestroyInstance(instance, None)
        if surface_extensions:
            glfw.terminate()
        msg = "no Vulkan physical device with a graphics queue was found"
        raise VulkanUnavailableError(msg)

    def descriptor(self) -> render.VulkanContextDescriptor:
        return render.VulkanContextDescriptor(
            instance=_pointer(self.instance, "VkInstance"),
            physical_device=_pointer(self.physical_device, "VkPhysicalDevice"),
            device=_pointer(self.device, "VkDevice"),
            graphics_queue=_pointer(self.queue, "VkQueue"),
            graphics_queue_family_index=self.queue_family_index,
            get_instance_proc_addr=_pointer(
                vk.ffi.addressof(vk.lib, "vkGetInstanceProcAddr"),
                "vkGetInstanceProcAddr",
            ),
            get_device_proc_addr=_pointer(
                vk.ffi.addressof(vk.lib, "vkGetDeviceProcAddr"),
                "vkGetDeviceProcAddr",
            ),
        )

    def owned_texture_descriptor(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> render.VulkanOwnedTextureDescriptor:
        return render.VulkanOwnedTextureDescriptor(
            extent=render.RenderTargetExtent(width, height, scale_factor),
            context=self.descriptor(),
        )

    def surface(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "VulkanSurface":
        if not self._glfw_initialized:
            msg = "VulkanContext must be created with surface_extensions=True"
            raise RuntimeError(msg)
        return VulkanSurface.create(self, width, height, scale_factor)

    def borrowed_image(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "VulkanBorrowedImage":
        return VulkanBorrowedImage.create(self, width, height, scale_factor)

    def close(self) -> None:
        if self._closed:
            return
        vk.vkDeviceWaitIdle(self.device)
        vk.vkDestroyDevice(self.device, None)
        vk.vkDestroyInstance(self.instance, None)
        if self._glfw_initialized:
            glfw.terminate()
        self._closed = True

    def __enter__(self) -> "VulkanContext":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class VulkanSurface:
    context: VulkanContext
    window: Any
    surface: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: VulkanContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "VulkanSurface":
        glfw.window_hint(glfw.CLIENT_API, glfw.NO_API)
        glfw.window_hint(glfw.VISIBLE, glfw.FALSE)
        window = glfw.create_window(
            width,
            height,
            "maplibre-native-python-test",
            None,
            None,
        )
        if not window:
            msg = "GLFW could not create a hidden Vulkan test window"
            raise VulkanUnavailableError(msg)
        try:
            out_surface = vk.ffi.new("VkSurfaceKHR *")
            result = glfw.create_window_surface(
                context.instance,
                window,
                None,
                out_surface,
            )
            if result != vk.VK_SUCCESS:
                msg = f"glfwCreateWindowSurface failed with VkResult {result}"
                raise VulkanUnavailableError(msg)
            return cls(context, window, out_surface[0], width, height, scale_factor)
        except Exception:
            glfw.destroy_window(window)
            raise

    def descriptor(self) -> render.VulkanSurfaceDescriptor:
        return render.VulkanSurfaceDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            surface=_pointer(self.surface, "VkSurfaceKHR"),
        )

    def close(self) -> None:
        if self._closed:
            return
        vk.lib.vkDestroySurfaceKHR(self.context.instance, self.surface, vk.ffi.NULL)
        glfw.destroy_window(self.window)
        self._closed = True

    def __enter__(self) -> "VulkanSurface":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class VulkanBorrowedImage:
    context: VulkanContext
    image: Any
    image_view: Any
    memory: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: VulkanContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "VulkanBorrowedImage":
        image = vk.vkCreateImage(
            context.device,
            vk.VkImageCreateInfo(
                imageType=vk.VK_IMAGE_TYPE_2D,
                format=vk.VK_FORMAT_R8G8B8A8_UNORM,
                extent=vk.VkExtent3D(width=width, height=height, depth=1),
                mipLevels=1,
                arrayLayers=1,
                samples=vk.VK_SAMPLE_COUNT_1_BIT,
                tiling=vk.VK_IMAGE_TILING_OPTIMAL,
                usage=(
                    vk.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    | vk.VK_IMAGE_USAGE_SAMPLED_BIT
                    | vk.VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                ),
                sharingMode=vk.VK_SHARING_MODE_EXCLUSIVE,
                initialLayout=vk.VK_IMAGE_LAYOUT_UNDEFINED,
            ),
            None,
        )
        memory = None
        image_view = None
        try:
            requirements = vk.vkGetImageMemoryRequirements(context.device, image)
            memory = vk.vkAllocateMemory(
                context.device,
                vk.VkMemoryAllocateInfo(
                    allocationSize=requirements.size,
                    memoryTypeIndex=_find_memory_type(
                        context.physical_device,
                        requirements.memoryTypeBits,
                        vk.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    ),
                ),
                None,
            )
            vk.vkBindImageMemory(context.device, image, memory, 0)
            image_view = vk.vkCreateImageView(
                context.device,
                vk.VkImageViewCreateInfo(
                    image=image,
                    viewType=vk.VK_IMAGE_VIEW_TYPE_2D,
                    format=vk.VK_FORMAT_R8G8B8A8_UNORM,
                    subresourceRange=vk.VkImageSubresourceRange(
                        aspectMask=vk.VK_IMAGE_ASPECT_COLOR_BIT,
                        baseMipLevel=0,
                        levelCount=1,
                        baseArrayLayer=0,
                        layerCount=1,
                    ),
                ),
                None,
            )
            return cls(context, image, image_view, memory, width, height, scale_factor)
        except Exception:
            if image_view is not None:
                vk.vkDestroyImageView(context.device, image_view, None)
            if memory is not None:
                vk.vkFreeMemory(context.device, memory, None)
            vk.vkDestroyImage(context.device, image, None)
            raise

    def descriptor(self) -> render.VulkanBorrowedTextureDescriptor:
        return render.VulkanBorrowedTextureDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            image=_pointer(self.image, "VkImage"),
            image_view=_pointer(self.image_view, "VkImageView"),
            format=vk.VK_FORMAT_R8G8B8A8_UNORM,
            initial_layout=vk.VK_IMAGE_LAYOUT_UNDEFINED,
            final_layout=vk.VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        )

    def close(self) -> None:
        if self._closed:
            return
        vk.vkDeviceWaitIdle(self.context.device)
        vk.vkDestroyImageView(self.context.device, self.image_view, None)
        vk.vkDestroyImage(self.context.device, self.image, None)
        vk.vkFreeMemory(self.context.device, self.memory, None)
        self._closed = True

    def __enter__(self) -> "VulkanBorrowedImage":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()
