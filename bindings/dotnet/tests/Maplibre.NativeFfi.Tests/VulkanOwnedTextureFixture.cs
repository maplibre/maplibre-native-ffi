using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Silk.NET.Core.Native;
using Silk.NET.Vulkan;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

internal sealed unsafe class VulkanOwnedTextureFixture : OwnedTextureFixture
{
    private const string PortabilityEnumeration = "VK_KHR_portability_enumeration";
    private const string PortabilitySubset = "VK_KHR_portability_subset";
    private readonly Vk vk;
    private Instance instance;
    private PhysicalDevice physicalDevice;
    private Device device;
    private Queue queue;
    private uint queueFamily;
    private bool disposed;

    internal static bool IsSupported =>
        Maplibre.SupportedRenderBackendMask().HasFlag(RenderBackendFlag.Vulkan);

    internal override bool SupportsFrameAcquisition => true;

    internal VulkanOwnedTextureFixture()
    {
        string[] names =
            OperatingSystem.IsWindows() ? ["vulkan-1.dll"]
            : OperatingSystem.IsMacOS()
                ? ["libvulkan.1.dylib", "libvulkan.dylib", "libMoltenVK.dylib"]
            : ["libvulkan.so.1"];
        string[] directories =
        [
            Environment.GetEnvironmentVariable("MLN_FFI_VULKAN_LOADER_DIR") ?? "",
            AppContext.BaseDirectory,
        ];
        var candidates = directories
            .Where(directory => directory.Length > 0)
            .SelectMany(directory => names.Select(name => Path.Combine(directory, name)))
            .Concat(names)
            .ToArray();
        vk = new Vk(Vk.CreateDefaultContext(candidates));
        try
        {
            CreateInstance();
            SelectGraphicsQueue();
            CreateDevice();
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    internal override RenderSessionHandle Attach(MapHandle map, RenderTargetExtent extent) =>
        map.VulkanOwnedTextureAttach(
            new VulkanOwnedTextureDescriptor { Extent = extent, Context = Context() },
            new RenderSessionAttachOptions
            {
                Driver = RenderDriverKind.CoreWorker,
                RequestedTextureRingDepth = 2,
            }
        );

    internal override Task SetTargetAsync(RenderSessionHandle session, RenderTargetExtent extent) =>
        // Owned textures reject retargeting before the native API reads the surface handle.
        session.VulkanSurfaceSetTargetAsync(
            new VulkanSurfaceDescriptor
            {
                Extent = extent,
                Context = Context(),
                Surface = 1,
            },
            TestContext.Current.CancellationToken
        );

    private VulkanContextDescriptor Context() =>
        new()
        {
            Instance = NativePointer.FromBorrowedAddress(instance.Handle),
            PhysicalDevice = NativePointer.FromBorrowedAddress(physicalDevice.Handle),
            Device = NativePointer.FromBorrowedAddress(device.Handle),
            GraphicsQueue = NativePointer.FromBorrowedAddress(queue.Handle),
            GraphicsQueueFamilyIndex = queueFamily,
            GetInstanceProcAddr = NativePointer.FromBorrowedAddress(
                (nint)vk.GetInstanceProcAddr(instance, "vkGetInstanceProcAddr")
            ),
            GetDeviceProcAddr = NativePointer.FromBorrowedAddress(
                (nint)vk.GetDeviceProcAddr(device, "vkGetDeviceProcAddr")
            ),
        };

    private void CreateInstance()
    {
        var portability = Extensions().Contains(PortabilityEnumeration);
        var extensionNames = SilkMarshal.StringArrayToPtr(
            portability ? [PortabilityEnumeration] : []
        );
        try
        {
            var application = new ApplicationInfo
            {
                SType = StructureType.ApplicationInfo,
                ApiVersion = Vk.Version10,
            };
            var options = new InstanceCreateInfo
            {
                SType = StructureType.InstanceCreateInfo,
                PApplicationInfo = &application,
                Flags = portability ? InstanceCreateFlags.EnumeratePortabilityBitKhr : 0,
                EnabledExtensionCount = portability ? 1u : 0u,
                PpEnabledExtensionNames = (byte**)extensionNames,
            };
            Check(vk.CreateInstance(&options, null, out instance), "vkCreateInstance");
        }
        finally
        {
            SilkMarshal.Free(extensionNames);
        }
    }

    private void SelectGraphicsQueue()
    {
        uint count = 0;
        Check(vk.EnumeratePhysicalDevices(instance, &count, null), "vkEnumeratePhysicalDevices");
        var devices = stackalloc PhysicalDevice[checked((int)count)];
        Check(vk.EnumeratePhysicalDevices(instance, &count, devices), "vkEnumeratePhysicalDevices");
        for (var index = 0; index < count; index++)
        {
            uint familyCount = 0;
            vk.GetPhysicalDeviceQueueFamilyProperties(devices[index], &familyCount, null);
            var families = new QueueFamilyProperties[checked((int)familyCount)];
            fixed (QueueFamilyProperties* pointer = families)
                vk.GetPhysicalDeviceQueueFamilyProperties(devices[index], &familyCount, pointer);
            for (uint family = 0; family < familyCount; family++)
            {
                if (
                    families[family].QueueCount > 0
                    && families[family].QueueFlags.HasFlag(QueueFlags.GraphicsBit)
                )
                {
                    physicalDevice = devices[index];
                    queueFamily = family;
                    return;
                }
            }
        }
        throw new InvalidOperationException("No Vulkan device has a graphics queue.");
    }

    private void CreateDevice()
    {
        var portability = Extensions(physicalDevice).Contains(PortabilitySubset);
        var extensionNames = SilkMarshal.StringArrayToPtr(portability ? [PortabilitySubset] : []);
        try
        {
            var priority = 1f;
            var queueInfo = new DeviceQueueCreateInfo
            {
                SType = StructureType.DeviceQueueCreateInfo,
                QueueFamilyIndex = queueFamily,
                QueueCount = 1,
                PQueuePriorities = &priority,
            };
            vk.GetPhysicalDeviceFeatures(physicalDevice, out var supported);
            var features = new PhysicalDeviceFeatures
            {
                SamplerAnisotropy = supported.SamplerAnisotropy,
                WideLines = supported.WideLines,
            };
            var options = new DeviceCreateInfo
            {
                SType = StructureType.DeviceCreateInfo,
                QueueCreateInfoCount = 1,
                PQueueCreateInfos = &queueInfo,
                EnabledExtensionCount = portability ? 1u : 0u,
                PpEnabledExtensionNames = (byte**)extensionNames,
                PEnabledFeatures = &features,
            };
            Check(vk.CreateDevice(physicalDevice, &options, null, out device), "vkCreateDevice");
            vk.GetDeviceQueue(device, queueFamily, 0, out queue);
        }
        finally
        {
            SilkMarshal.Free(extensionNames);
        }
    }

    private HashSet<string> Extensions(PhysicalDevice target = default)
    {
        uint count = 0;
        Check(
            target.Handle == 0
                ? vk.EnumerateInstanceExtensionProperties((byte*)null, &count, null)
                : vk.EnumerateDeviceExtensionProperties(target, (byte*)null, &count, null),
            "Vulkan extension count"
        );
        var properties = stackalloc ExtensionProperties[checked((int)count)];
        Check(
            target.Handle == 0
                ? vk.EnumerateInstanceExtensionProperties((byte*)null, &count, properties)
                : vk.EnumerateDeviceExtensionProperties(target, (byte*)null, &count, properties),
            "Vulkan extensions"
        );
        var names = new HashSet<string>(StringComparer.Ordinal);
        for (var index = 0; index < count; index++)
            names.Add(Marshal.PtrToStringUTF8((nint)properties[index].ExtensionName)!);
        return names;
    }

    private static void Check(Result result, string operation)
    {
        if (result != Result.Success)
            throw new InvalidOperationException($"{operation} failed with {result}.");
    }

    public override void Dispose()
    {
        if (disposed)
            return;
        disposed = true;
        if (device.Handle != 0)
        {
            vk.DeviceWaitIdle(device);
            vk.DestroyDevice(device, null);
        }
        if (instance.Handle != 0)
            vk.DestroyInstance(instance, null);
        vk.Dispose();
    }
}
