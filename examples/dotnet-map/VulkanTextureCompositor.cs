using System.Runtime.InteropServices;
using Maplibre.Native.Render;
using Silk.NET.Core;
using Silk.NET.Vulkan;
using VulkanSemaphore = Silk.NET.Vulkan.Semaphore;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe partial class VulkanTextureCompositor : ITextureCompositor
{
    private static readonly byte[] VertexShader = Convert.FromBase64String(
        "AwIjBwAAAQALAAgAMwAAAAAAAAARAAIAAQAAAAsABgABAAAAR0xTTC5zdGQuNDUwAAAAAA4AAwAAAAAAAQAAAA8ACAAAAAAABAAAAG1haW4AAAAAHwAAACMAAAAvAAAAAwADAAIAAADCAQAABQAEAAQAAABtYWluAAAAAAUABQAMAAAAcG9zaXRpb25zAAAABQADABMAAAB1dnMABQAGAB0AAABnbF9QZXJWZXJ0ZXgAAAAABgAGAB0AAAAAAAAAZ2xfUG9zaXRpb24ABgAHAB0AAAABAAAAZ2xfUG9pbnRTaXplAAAAAAYABwAdAAAAAgAAAGdsX0NsaXBEaXN0YW5jZQAGAAcAHQAAAAMAAABnbF9DdWxsRGlzdGFuY2UABQADAB8AAAAAAAAABQAGACMAAABnbF9WZXJ0ZXhJbmRleAAABQAEAC8AAABvdXRfdXYAAEcAAwAdAAAAAgAAAEgABQAdAAAAAAAAAAsAAAAAAAAASAAFAB0AAAABAAAACwAAAAEAAABIAAUAHQAAAAIAAAALAAAAAwAAAEgABQAdAAAAAwAAAAsAAAAEAAAARwAEACMAAAALAAAAKgAAAEcABAAvAAAAHgAAAAAAAAATAAIAAgAAACEAAwADAAAAAgAAABYAAwAGAAAAIAAAABcABAAHAAAABgAAAAIAAAAVAAQACAAAACAAAAAAAAAAKwAEAAgAAAAJAAAAAwAAABwABAAKAAAABwAAAAkAAAAgAAQACwAAAAYAAAAKAAAAOwAEAAsAAAAMAAAABgAAACsABAAGAAAADQAAAAAAgL8sAAUABwAAAA4AAAANAAAADQAAACsABAAGAAAADwAAAAAAQEAsAAUABwAAABAAAAAPAAAADQAAACwABQAHAAAAEQAAAA0AAAAPAAAALAAGAAoAAAASAAAADgAAABAAAAARAAAAOwAEAAsAAAATAAAABgAAACsABAAGAAAAFAAAAAAAAAAsAAUABwAAABUAAAAUAAAAFAAAACsABAAGAAAAFgAAAAAAAEAsAAUABwAAABcAAAAWAAAAFAAAACwABQAHAAAAGAAAABQAAAAWAAAALAAGAAoAAAAZAAAAFQAAABcAAAAYAAAAFwAEABoAAAAGAAAABAAAACsABAAIAAAAGwAAAAEAAAAcAAQAHAAAAAYAAAAbAAAAHgAGAB0AAAAaAAAABgAAABwAAAAcAAAAIAAEAB4AAAADAAAAHQAAADsABAAeAAAAHwAAAAMAAAAVAAQAIAAAACAAAAABAAAAKwAEACAAAAAhAAAAAAAAACAABAAiAAAAAQAAACAAAAA7AAQAIgAAACMAAAABAAAAIAAEACUAAAAGAAAABwAAACsABAAGAAAAKAAAAAAAgD8gAAQALAAAAAMAAAAaAAAAIAAEAC4AAAADAAAABwAAADsABAAuAAAALwAAAAMAAAA2AAUAAgAAAAQAAAAAAAAAAwAAAPgAAgAFAAAAPgADAAwAAAASAAAAPgADABMAAAAZAAAAPQAEACAAAAAkAAAAIwAAAEEABQAlAAAAJgAAAAwAAAAkAAAAPQAEAAcAAAAnAAAAJgAAAFEABQAGAAAAKQAAACcAAAAAAAAAUQAFAAYAAAAqAAAAJwAAAAEAAABQAAcAGgAAACsAAAApAAAAKgAAABQAAAAoAAAAQQAFACwAAAAtAAAAHwAAACEAAAA+AAMALQAAACsAAAA9AAQAIAAAADAAAAAjAAAAQQAFACUAAAAxAAAAEwAAADAAAAA9AAQABwAAADIAAAAxAAAAPgADAC8AAAAyAAAA/QABADgAAQA="
    );

    private static readonly byte[] FragmentShader = Convert.FromBase64String(
        "AwIjBwAAAQALAAgAFAAAAAAAAAARAAIAAQAAAAsABgABAAAAR0xTTC5zdGQuNDUwAAAAAA4AAwAAAAAAAQAAAA8ABwAEAAAABAAAAG1haW4AAAAACQAAABEAAAAQAAMABAAAAAcAAAADAAMAAgAAAMIBAAAFAAQABAAAAG1haW4AAAAABQAFAAkAAABvdXRfY29sb3IAAAAFAAUADQAAAG1hcF90ZXh0dXJlAAUABAARAAAAaW5fdXYAAABHAAQACQAAAB4AAAAAAAAARwAEAA0AAAAhAAAAAAAAAEcABAANAAAAIgAAAAAAAABHAAQAEQAAAB4AAAAAAAAAEwACAAIAAAAhAAMAAwAAAAIAAAAWAAMABgAAACAAAAAXAAQABwAAAAYAAAAEAAAAIAAEAAgAAAADAAAABwAAADsABAAIAAAACQAAAAMAAAAZAAkACgAAAAYAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAbAAMACwAAAAoAAAAgAAQADAAAAAAAAAALAAAAOwAEAAwAAAANAAAAAAAAABcABAAPAAAABgAAAAIAAAAgAAQAEAAAAAEAAAAPAAAAOwAEABAAAAARAAAAAQAAADYABQACAAAABAAAAAAAAAADAAAA+AACAAUAAAA9AAQACwAAAA4AAAANAAAAPQAEAA8AAAASAAAAEQAAAFcABQAHAAAAEwAAAA4AAAASAAAAPgADAAkAAAATAAAA/QABADgAAQA="
    );

    private readonly VulkanContext context;
    private readonly Vk vk;
    private SwapchainKHR swapchain;
    private Format swapchainFormat;
    private Extent2D extent;
    private ImageView[] imageViews = [];
    private Framebuffer[] framebuffers = [];
    private RenderPass renderPass;
    private DescriptorSetLayout descriptorSetLayout;
    private PipelineLayout pipelineLayout;
    private Pipeline pipeline;
    private Sampler sampler;
    private DescriptorPool descriptorPool;
    private DescriptorSet descriptorSet;
    private CommandPool commandPool;
    private CommandBuffer commandBuffer;
    private VulkanSemaphore imageAvailable;
    private VulkanSemaphore renderFinished;
    private Fence inFlight;
    private Viewport viewport;

    public VulkanTextureCompositor(VulkanContext context, Viewport viewport)
    {
        this.context = context;
        this.viewport = viewport;
        vk = context.Api;
        try
        {
            CreateSwapchain(viewport, default);
            CreateRenderPass();
            CreateDescriptorState();
            CreatePipeline();
            CreateFramebuffers();
            CreateCommands();
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    public void Resize(Viewport viewport)
    {
        this.viewport = viewport;
        context.WaitIdle();
        DestroySwapchainDependents();
        var oldSwapchain = swapchain;
        var oldFormat = swapchainFormat;
        swapchain = default;
        try
        {
            CreateSwapchain(viewport, oldSwapchain);
        }
        finally
        {
            if (oldSwapchain.Handle != 0)
            {
                vkDestroySwapchainKHR(context.Device.Handle, oldSwapchain.Handle, 0);
            }
        }

        if (renderPass.Handle == 0 || swapchainFormat != oldFormat)
        {
            DestroyPipeline();
            DestroyPipelineLayout();
            DestroyRenderPass();
            CreateRenderPass();
            CreatePipeline();
        }

        CreateFramebuffers();
    }

    public bool Draw(VulkanOwnedTextureFrame frame)
    {
        if (frame.Width == 0 || frame.Height == 0)
        {
            throw new InvalidOperationException(
                "MapLibre returned an empty Vulkan owned texture frame."
            );
        }

        if (frame.Layout != (uint)ImageLayout.ShaderReadOnlyOptimal)
        {
            throw new InvalidOperationException(
                $"MapLibre owned texture frame is not shader-readable: layout={frame.Layout}."
            );
        }

        if (frame.ImageView.IsNull)
        {
            throw new InvalidOperationException("MapLibre returned a null Vulkan image view.");
        }

        return DrawImageView(new ImageView((ulong)frame.ImageView.Address));
    }

    public bool DrawImageView(ImageView imageView)
    {
        var fence = inFlight;
        VulkanContext.Check(
            vk.WaitForFences(context.Device, 1, &fence, true, ulong.MaxValue),
            "vkWaitForFences"
        );
        var imageIndex = 0u;
        var acquire = vkAcquireNextImageKHR(
            context.Device.Handle,
            swapchain.Handle,
            ulong.MaxValue,
            imageAvailable.Handle,
            0,
            &imageIndex
        );
        if (acquire == Result.ErrorOutOfDateKhr)
        {
            RecreateSwapchain();
            return false;
        }

        if (acquire != Result.Success && acquire != Result.SuboptimalKhr)
        {
            VulkanContext.Check(acquire, "vkAcquireNextImageKHR");
        }

        UpdateDescriptor(imageView);
        Record(imageIndex);
        VulkanContext.Check(vk.ResetFences(context.Device, 1, &fence), "vkResetFences");
        Submit();
        fence = inFlight;
        VulkanContext.Check(
            vk.WaitForFences(context.Device, 1, &fence, true, ulong.MaxValue),
            "vkWaitForFences"
        );
        var present = Present(imageIndex);
        VulkanContext.Check(vk.QueueWaitIdle(context.GraphicsQueue), "vkQueueWaitIdle");
        if (present == Result.ErrorOutOfDateKhr)
        {
            RecreateSwapchain();
            return false;
        }

        return true;
    }

    public void Dispose()
    {
        if (context.Device.Handle != 0)
        {
            context.WaitIdle();
        }

        if (inFlight.Handle != 0)
        {
            vk.DestroyFence(context.Device, inFlight, null);
            inFlight = default;
        }

        if (renderFinished.Handle != 0)
        {
            vk.DestroySemaphore(context.Device, renderFinished, null);
            renderFinished = default;
        }

        if (imageAvailable.Handle != 0)
        {
            vk.DestroySemaphore(context.Device, imageAvailable, null);
            imageAvailable = default;
        }

        if (commandPool.Handle != 0)
        {
            vk.DestroyCommandPool(context.Device, commandPool, null);
            commandPool = default;
            commandBuffer = default;
        }

        DestroySwapchainDependents();
        if (swapchain.Handle != 0)
        {
            vkDestroySwapchainKHR(context.Device.Handle, swapchain.Handle, 0);
            swapchain = default;
        }

        if (pipeline.Handle != 0)
        {
            DestroyPipeline();
        }

        if (pipelineLayout.Handle != 0)
        {
            DestroyPipelineLayout();
        }

        if (descriptorPool.Handle != 0)
        {
            vk.DestroyDescriptorPool(context.Device, descriptorPool, null);
            descriptorPool = default;
            descriptorSet = default;
        }

        if (sampler.Handle != 0)
        {
            vk.DestroySampler(context.Device, sampler, null);
            sampler = default;
        }

        if (descriptorSetLayout.Handle != 0)
        {
            vk.DestroyDescriptorSetLayout(context.Device, descriptorSetLayout, null);
            descriptorSetLayout = default;
        }

        if (renderPass.Handle != 0)
        {
            DestroyRenderPass();
        }
    }

    private void CreateSwapchain(Viewport viewport, SwapchainKHR oldSwapchain)
    {
        SurfaceCapabilitiesKHR capabilities;
        VulkanContext.Check(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                context.PhysicalDevice.Handle,
                context.Surface.Handle,
                &capabilities
            ),
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"
        );
        uint formatCount = 0;
        VulkanContext.Check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                context.PhysicalDevice.Handle,
                context.Surface.Handle,
                &formatCount,
                null
            ),
            "vkGetPhysicalDeviceSurfaceFormatsKHR(count)"
        );
        if (formatCount == 0)
        {
            throw new InvalidOperationException("Vulkan surface reported no formats.");
        }

        var formats = stackalloc SurfaceFormatKHR[checked((int)formatCount)];
        VulkanContext.Check(
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                context.PhysicalDevice.Handle,
                context.Surface.Handle,
                &formatCount,
                formats
            ),
            "vkGetPhysicalDeviceSurfaceFormatsKHR"
        );
        var chosen = ChooseSurfaceFormat(formats, formatCount);

        swapchainFormat = chosen.Format;
        extent = ChooseExtent(capabilities, viewport);
        var imageCount = capabilities.MinImageCount + 1;
        if (capabilities.MaxImageCount > 0 && imageCount > capabilities.MaxImageCount)
        {
            imageCount = capabilities.MaxImageCount;
        }

        var createInfo = new SwapchainCreateInfoKHR
        {
            SType = StructureType.SwapchainCreateInfoKhr,
            Surface = context.Surface,
            MinImageCount = imageCount,
            ImageFormat = swapchainFormat,
            ImageColorSpace = chosen.ColorSpace,
            ImageExtent = extent,
            ImageArrayLayers = 1,
            ImageUsage = ImageUsageFlags.ColorAttachmentBit,
            ImageSharingMode = SharingMode.Exclusive,
            PreTransform = capabilities.CurrentTransform,
            CompositeAlpha = CompositeAlphaFlagsKHR.OpaqueBitKhr,
            PresentMode = PresentModeKHR.FifoKhr,
            Clipped = true,
            OldSwapchain = oldSwapchain,
        };
        ulong outSwapchain = 0;
        VulkanContext.Check(
            vkCreateSwapchainKHR(context.Device.Handle, &createInfo, 0, &outSwapchain),
            "vkCreateSwapchainKHR"
        );
        swapchain = new SwapchainKHR(outSwapchain);

        uint actualCount = 0;
        VulkanContext.Check(
            vkGetSwapchainImagesKHR(context.Device.Handle, swapchain.Handle, &actualCount, null),
            "vkGetSwapchainImagesKHR(count)"
        );
        var images = stackalloc Image[checked((int)actualCount)];
        VulkanContext.Check(
            vkGetSwapchainImagesKHR(context.Device.Handle, swapchain.Handle, &actualCount, images),
            "vkGetSwapchainImagesKHR"
        );
        imageViews = new ImageView[actualCount];
        try
        {
            for (var i = 0; i < actualCount; i++)
            {
                imageViews[i] = CreateImageView(images[i], swapchainFormat);
            }
        }
        catch
        {
            DestroySwapchainDependents();
            vkDestroySwapchainKHR(context.Device.Handle, swapchain.Handle, 0);
            swapchain = default;
            throw;
        }
    }

    private void CreateRenderPass()
    {
        var attachment = new AttachmentDescription
        {
            Format = swapchainFormat,
            Samples = SampleCountFlags.Count1Bit,
            LoadOp = AttachmentLoadOp.Clear,
            StoreOp = AttachmentStoreOp.Store,
            StencilLoadOp = AttachmentLoadOp.DontCare,
            StencilStoreOp = AttachmentStoreOp.DontCare,
            InitialLayout = ImageLayout.Undefined,
            FinalLayout = ImageLayout.PresentSrcKhr,
        };
        var colorReference = new AttachmentReference
        {
            Attachment = 0,
            Layout = ImageLayout.ColorAttachmentOptimal,
        };
        var dependency = new SubpassDependency
        {
            SrcSubpass = Vk.SubpassExternal,
            DstSubpass = 0,
            SrcStageMask = PipelineStageFlags.ColorAttachmentOutputBit,
            DstStageMask = PipelineStageFlags.ColorAttachmentOutputBit,
            DstAccessMask = AccessFlags.ColorAttachmentWriteBit,
        };
        var subpass = new SubpassDescription
        {
            PipelineBindPoint = PipelineBindPoint.Graphics,
            ColorAttachmentCount = 1,
            PColorAttachments = &colorReference,
        };
        var renderPassInfo = new RenderPassCreateInfo
        {
            SType = StructureType.RenderPassCreateInfo,
            AttachmentCount = 1,
            PAttachments = &attachment,
            SubpassCount = 1,
            PSubpasses = &subpass,
            DependencyCount = 1,
            PDependencies = &dependency,
        };
        VulkanContext.Check(
            vk.CreateRenderPass(context.Device, &renderPassInfo, null, out renderPass),
            "vkCreateRenderPass"
        );
    }

    private void CreateDescriptorState()
    {
        var binding = new DescriptorSetLayoutBinding
        {
            Binding = 0,
            DescriptorType = DescriptorType.CombinedImageSampler,
            DescriptorCount = 1,
            StageFlags = ShaderStageFlags.FragmentBit,
        };
        var layoutInfo = new DescriptorSetLayoutCreateInfo
        {
            SType = StructureType.DescriptorSetLayoutCreateInfo,
            BindingCount = 1,
            PBindings = &binding,
        };
        VulkanContext.Check(
            vk.CreateDescriptorSetLayout(
                context.Device,
                &layoutInfo,
                null,
                out descriptorSetLayout
            ),
            "vkCreateDescriptorSetLayout"
        );

        var samplerInfo = new SamplerCreateInfo
        {
            SType = StructureType.SamplerCreateInfo,
            MagFilter = Filter.Linear,
            MinFilter = Filter.Linear,
            MipmapMode = SamplerMipmapMode.Linear,
            AddressModeU = SamplerAddressMode.ClampToEdge,
            AddressModeV = SamplerAddressMode.ClampToEdge,
            AddressModeW = SamplerAddressMode.ClampToEdge,
            MaxAnisotropy = 1,
            CompareOp = CompareOp.Always,
            BorderColor = BorderColor.IntOpaqueBlack,
        };
        VulkanContext.Check(
            vk.CreateSampler(context.Device, &samplerInfo, null, out sampler),
            "vkCreateSampler"
        );

        var poolSize = new DescriptorPoolSize
        {
            Type = DescriptorType.CombinedImageSampler,
            DescriptorCount = 1,
        };
        var poolInfo = new DescriptorPoolCreateInfo
        {
            SType = StructureType.DescriptorPoolCreateInfo,
            MaxSets = 1,
            PoolSizeCount = 1,
            PPoolSizes = &poolSize,
        };
        VulkanContext.Check(
            vk.CreateDescriptorPool(context.Device, &poolInfo, null, out descriptorPool),
            "vkCreateDescriptorPool"
        );

        var layout = descriptorSetLayout;
        var allocateInfo = new DescriptorSetAllocateInfo
        {
            SType = StructureType.DescriptorSetAllocateInfo,
            DescriptorPool = descriptorPool,
            DescriptorSetCount = 1,
            PSetLayouts = &layout,
        };
        VulkanContext.Check(
            vk.AllocateDescriptorSets(context.Device, &allocateInfo, out descriptorSet),
            "vkAllocateDescriptorSets"
        );
    }

    private void CreatePipeline()
    {
        var vertex = CreateShaderModule(VertexShader);
        var fragment = CreateShaderModule(FragmentShader);
        var main = stackalloc byte[] { (byte)'m', (byte)'a', (byte)'i', (byte)'n', 0 };
        try
        {
            var stages = stackalloc PipelineShaderStageCreateInfo[2];
            stages[0] = new PipelineShaderStageCreateInfo
            {
                SType = StructureType.PipelineShaderStageCreateInfo,
                Stage = ShaderStageFlags.VertexBit,
                Module = vertex,
                PName = main,
            };
            stages[1] = new PipelineShaderStageCreateInfo
            {
                SType = StructureType.PipelineShaderStageCreateInfo,
                Stage = ShaderStageFlags.FragmentBit,
                Module = fragment,
                PName = main,
            };
            var vertexInput = new PipelineVertexInputStateCreateInfo
            {
                SType = StructureType.PipelineVertexInputStateCreateInfo,
            };
            var inputAssembly = new PipelineInputAssemblyStateCreateInfo
            {
                SType = StructureType.PipelineInputAssemblyStateCreateInfo,
                Topology = PrimitiveTopology.TriangleList,
            };
            var viewportState = new PipelineViewportStateCreateInfo
            {
                SType = StructureType.PipelineViewportStateCreateInfo,
                ViewportCount = 1,
                ScissorCount = 1,
            };
            var rasterizer = new PipelineRasterizationStateCreateInfo
            {
                SType = StructureType.PipelineRasterizationStateCreateInfo,
                PolygonMode = PolygonMode.Fill,
                CullMode = CullModeFlags.None,
                FrontFace = FrontFace.CounterClockwise,
                LineWidth = 1,
            };
            var multisample = new PipelineMultisampleStateCreateInfo
            {
                SType = StructureType.PipelineMultisampleStateCreateInfo,
                RasterizationSamples = SampleCountFlags.Count1Bit,
            };
            var colorBlendAttachment = new PipelineColorBlendAttachmentState
            {
                ColorWriteMask =
                    ColorComponentFlags.RBit
                    | ColorComponentFlags.GBit
                    | ColorComponentFlags.BBit
                    | ColorComponentFlags.ABit,
            };
            var colorBlend = new PipelineColorBlendStateCreateInfo
            {
                SType = StructureType.PipelineColorBlendStateCreateInfo,
                AttachmentCount = 1,
                PAttachments = &colorBlendAttachment,
            };
            var dynamicStates = stackalloc DynamicState[]
            {
                DynamicState.Viewport,
                DynamicState.Scissor,
            };
            var dynamicState = new PipelineDynamicStateCreateInfo
            {
                SType = StructureType.PipelineDynamicStateCreateInfo,
                DynamicStateCount = 2,
                PDynamicStates = dynamicStates,
            };
            var setLayout = descriptorSetLayout;
            var layoutInfo = new PipelineLayoutCreateInfo
            {
                SType = StructureType.PipelineLayoutCreateInfo,
                SetLayoutCount = 1,
                PSetLayouts = &setLayout,
            };
            VulkanContext.Check(
                vk.CreatePipelineLayout(context.Device, &layoutInfo, null, out pipelineLayout),
                "vkCreatePipelineLayout"
            );
            var pipelineInfo = new GraphicsPipelineCreateInfo
            {
                SType = StructureType.GraphicsPipelineCreateInfo,
                StageCount = 2,
                PStages = stages,
                PVertexInputState = &vertexInput,
                PInputAssemblyState = &inputAssembly,
                PViewportState = &viewportState,
                PRasterizationState = &rasterizer,
                PMultisampleState = &multisample,
                PColorBlendState = &colorBlend,
                PDynamicState = &dynamicState,
                Layout = pipelineLayout,
                RenderPass = renderPass,
                Subpass = 0,
            };
            VulkanContext.Check(
                vk.CreateGraphicsPipelines(
                    context.Device,
                    default,
                    1,
                    &pipelineInfo,
                    null,
                    out pipeline
                ),
                "vkCreateGraphicsPipelines"
            );
        }
        finally
        {
            vk.DestroyShaderModule(context.Device, fragment, null);
            vk.DestroyShaderModule(context.Device, vertex, null);
        }
    }

    private void CreateFramebuffers()
    {
        framebuffers = new Framebuffer[imageViews.Length];
        try
        {
            for (var i = 0; i < imageViews.Length; i++)
            {
                var attachment = imageViews[i];
                var info = new FramebufferCreateInfo
                {
                    SType = StructureType.FramebufferCreateInfo,
                    RenderPass = renderPass,
                    AttachmentCount = 1,
                    PAttachments = &attachment,
                    Width = extent.Width,
                    Height = extent.Height,
                    Layers = 1,
                };
                VulkanContext.Check(
                    vk.CreateFramebuffer(context.Device, &info, null, out framebuffers[i]),
                    "vkCreateFramebuffer"
                );
            }
        }
        catch
        {
            DestroySwapchainDependents();
            throw;
        }
    }

    private void CreateCommands()
    {
        var poolInfo = new CommandPoolCreateInfo
        {
            SType = StructureType.CommandPoolCreateInfo,
            Flags = CommandPoolCreateFlags.ResetCommandBufferBit,
            QueueFamilyIndex = context.GraphicsQueueFamilyIndex,
        };
        VulkanContext.Check(
            vk.CreateCommandPool(context.Device, &poolInfo, null, out commandPool),
            "vkCreateCommandPool"
        );
        var allocateInfo = new CommandBufferAllocateInfo
        {
            SType = StructureType.CommandBufferAllocateInfo,
            CommandPool = commandPool,
            Level = CommandBufferLevel.Primary,
            CommandBufferCount = 1,
        };
        VulkanContext.Check(
            vk.AllocateCommandBuffers(context.Device, &allocateInfo, out commandBuffer),
            "vkAllocateCommandBuffers"
        );
        var semaphoreInfo = new SemaphoreCreateInfo { SType = StructureType.SemaphoreCreateInfo };
        VulkanContext.Check(
            vk.CreateSemaphore(context.Device, &semaphoreInfo, null, out imageAvailable),
            "vkCreateSemaphore(imageAvailable)"
        );
        VulkanContext.Check(
            vk.CreateSemaphore(context.Device, &semaphoreInfo, null, out renderFinished),
            "vkCreateSemaphore(renderFinished)"
        );
        var fenceInfo = new FenceCreateInfo
        {
            SType = StructureType.FenceCreateInfo,
            Flags = FenceCreateFlags.SignaledBit,
        };
        VulkanContext.Check(
            vk.CreateFence(context.Device, &fenceInfo, null, out inFlight),
            "vkCreateFence"
        );
    }

    private void UpdateDescriptor(ImageView imageView)
    {
        var imageInfo = new DescriptorImageInfo
        {
            Sampler = sampler,
            ImageView = imageView,
            ImageLayout = ImageLayout.ShaderReadOnlyOptimal,
        };
        var write = new WriteDescriptorSet
        {
            SType = StructureType.WriteDescriptorSet,
            DstSet = descriptorSet,
            DstBinding = 0,
            DescriptorCount = 1,
            DescriptorType = DescriptorType.CombinedImageSampler,
            PImageInfo = &imageInfo,
        };
        vk.UpdateDescriptorSets(context.Device, 1, &write, 0, null);
    }

    private void Record(uint imageIndex)
    {
        VulkanContext.Check(
            vk.ResetCommandBuffer(commandBuffer, CommandBufferResetFlags.None),
            "vkResetCommandBuffer"
        );
        var beginInfo = new CommandBufferBeginInfo { SType = StructureType.CommandBufferBeginInfo };
        VulkanContext.Check(
            vk.BeginCommandBuffer(commandBuffer, &beginInfo),
            "vkBeginCommandBuffer"
        );
        var clear = new ClearValue { Color = new ClearColorValue(0.08f, 0.09f, 0.11f, 1.0f) };
        var renderArea = new Rect2D { Extent = extent };
        var renderPassInfo = new RenderPassBeginInfo
        {
            SType = StructureType.RenderPassBeginInfo,
            RenderPass = renderPass,
            Framebuffer = framebuffers[imageIndex],
            RenderArea = renderArea,
            ClearValueCount = 1,
            PClearValues = &clear,
        };
        vk.CmdBeginRenderPass(commandBuffer, &renderPassInfo, SubpassContents.Inline);
        var viewport = new Silk.NET.Vulkan.Viewport
        {
            X = 0,
            Y = 0,
            Width = extent.Width,
            Height = extent.Height,
            MinDepth = 0,
            MaxDepth = 1,
        };
        var scissor = renderArea;
        vk.CmdBindPipeline(commandBuffer, PipelineBindPoint.Graphics, pipeline);
        vk.CmdSetViewport(commandBuffer, 0, 1, &viewport);
        vk.CmdSetScissor(commandBuffer, 0, 1, &scissor);
        var set = descriptorSet;
        vk.CmdBindDescriptorSets(
            commandBuffer,
            PipelineBindPoint.Graphics,
            pipelineLayout,
            0,
            1,
            &set,
            0,
            null
        );
        vk.CmdDraw(commandBuffer, 3, 1, 0, 0);
        vk.CmdEndRenderPass(commandBuffer);
        VulkanContext.Check(vk.EndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
    }

    private void Submit()
    {
        var waitStage = PipelineStageFlags.ColorAttachmentOutputBit;
        var waitSemaphore = imageAvailable;
        var signalSemaphore = renderFinished;
        var buffer = commandBuffer;
        var submitInfo = new SubmitInfo
        {
            SType = StructureType.SubmitInfo,
            WaitSemaphoreCount = 1,
            PWaitSemaphores = &waitSemaphore,
            PWaitDstStageMask = &waitStage,
            CommandBufferCount = 1,
            PCommandBuffers = &buffer,
            SignalSemaphoreCount = 1,
            PSignalSemaphores = &signalSemaphore,
        };
        VulkanContext.Check(
            vk.QueueSubmit(context.GraphicsQueue, 1, &submitInfo, inFlight),
            "vkQueueSubmit"
        );
    }

    private void RecreateSwapchain()
    {
        Resize(viewport);
    }

    private Result Present(uint imageIndex)
    {
        var waitSemaphore = renderFinished;
        var swapchainHandle = swapchain;
        var presentInfo = new PresentInfoKHR
        {
            SType = StructureType.PresentInfoKhr,
            WaitSemaphoreCount = 1,
            PWaitSemaphores = &waitSemaphore,
            SwapchainCount = 1,
            PSwapchains = &swapchainHandle,
            PImageIndices = &imageIndex,
        };
        var result = vkQueuePresentKHR(context.GraphicsQueue.Handle, &presentInfo);
        if (
            result != Result.Success
            && result != Result.SuboptimalKhr
            && result != Result.ErrorOutOfDateKhr
        )
        {
            VulkanContext.Check(result, "vkQueuePresentKHR");
        }

        return result;
    }

    private static SurfaceFormatKHR ChooseSurfaceFormat(SurfaceFormatKHR* formats, uint formatCount)
    {
        for (var i = 0; i < formatCount; i++)
        {
            var candidate = formats[i];
            if (
                (
                    candidate.Format == Format.B8G8R8A8Unorm
                    || candidate.Format == Format.R8G8B8A8Unorm
                )
                && candidate.ColorSpace == ColorSpaceKHR.PaceSrgbNonlinearKhr
            )
            {
                return candidate;
            }
        }

        var first = formats[0];
        return first.Format == Format.Undefined
            ? new SurfaceFormatKHR { Format = Format.B8G8R8A8Unorm, ColorSpace = first.ColorSpace }
            : first;
    }

    private ImageView CreateImageView(Image image, Format format)
    {
        var subresourceRange = new ImageSubresourceRange
        {
            AspectMask = ImageAspectFlags.ColorBit,
            BaseMipLevel = 0,
            LevelCount = 1,
            BaseArrayLayer = 0,
            LayerCount = 1,
        };
        var info = new ImageViewCreateInfo
        {
            SType = StructureType.ImageViewCreateInfo,
            Image = image,
            ViewType = ImageViewType.Type2D,
            Format = format,
            SubresourceRange = subresourceRange,
        };
        VulkanContext.Check(
            vk.CreateImageView(context.Device, &info, null, out var imageView),
            "vkCreateImageView"
        );
        return imageView;
    }

    private ShaderModule CreateShaderModule(byte[] bytes)
    {
        fixed (byte* code = bytes)
        {
            var info = new ShaderModuleCreateInfo
            {
                SType = StructureType.ShaderModuleCreateInfo,
                CodeSize = (nuint)bytes.Length,
                PCode = (uint*)code,
            };
            VulkanContext.Check(
                vk.CreateShaderModule(context.Device, &info, null, out var module),
                "vkCreateShaderModule"
            );
            return module;
        }
    }

    private void DestroySwapchainDependents()
    {
        foreach (var framebuffer in framebuffers)
        {
            if (framebuffer.Handle != 0)
            {
                vk.DestroyFramebuffer(context.Device, framebuffer, null);
            }
        }

        framebuffers = [];
        foreach (var imageView in imageViews)
        {
            if (imageView.Handle != 0)
            {
                vk.DestroyImageView(context.Device, imageView, null);
            }
        }

        imageViews = [];
    }

    private void DestroyPipeline()
    {
        if (pipeline.Handle != 0)
        {
            vk.DestroyPipeline(context.Device, pipeline, null);
            pipeline = default;
        }
    }

    private void DestroyPipelineLayout()
    {
        if (pipelineLayout.Handle != 0)
        {
            vk.DestroyPipelineLayout(context.Device, pipelineLayout, null);
            pipelineLayout = default;
        }
    }

    private void DestroyRenderPass()
    {
        if (renderPass.Handle != 0)
        {
            vk.DestroyRenderPass(context.Device, renderPass, null);
            renderPass = default;
        }
    }

    private static Extent2D ChooseExtent(SurfaceCapabilitiesKHR capabilities, Viewport viewport)
    {
        if (capabilities.CurrentExtent.Width != uint.MaxValue)
        {
            return capabilities.CurrentExtent;
        }

        return new Extent2D(
            Math.Clamp(
                viewport.PhysicalWidth,
                capabilities.MinImageExtent.Width,
                capabilities.MaxImageExtent.Width
            ),
            Math.Clamp(
                viewport.PhysicalHeight,
                capabilities.MinImageExtent.Height,
                capabilities.MaxImageExtent.Height
            )
        );
    }

    [LibraryImport("vulkan", EntryPoint = "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")]
    private static partial Result vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        nint physicalDevice,
        ulong surface,
        SurfaceCapabilitiesKHR* capabilities
    );

    [LibraryImport("vulkan", EntryPoint = "vkGetPhysicalDeviceSurfaceFormatsKHR")]
    private static partial Result vkGetPhysicalDeviceSurfaceFormatsKHR(
        nint physicalDevice,
        ulong surface,
        uint* count,
        SurfaceFormatKHR* formats
    );

    [LibraryImport("vulkan", EntryPoint = "vkCreateSwapchainKHR")]
    private static partial Result vkCreateSwapchainKHR(
        nint device,
        SwapchainCreateInfoKHR* createInfo,
        nint allocator,
        ulong* swapchain
    );

    [LibraryImport("vulkan", EntryPoint = "vkDestroySwapchainKHR")]
    private static partial void vkDestroySwapchainKHR(nint device, ulong swapchain, nint allocator);

    [LibraryImport("vulkan", EntryPoint = "vkGetSwapchainImagesKHR")]
    private static partial Result vkGetSwapchainImagesKHR(
        nint device,
        ulong swapchain,
        uint* count,
        Image* images
    );

    [LibraryImport("vulkan", EntryPoint = "vkAcquireNextImageKHR")]
    private static partial Result vkAcquireNextImageKHR(
        nint device,
        ulong swapchain,
        ulong timeout,
        ulong semaphore,
        ulong fence,
        uint* imageIndex
    );

    [LibraryImport("vulkan", EntryPoint = "vkQueuePresentKHR")]
    private static partial Result vkQueuePresentKHR(nint queue, PresentInfoKHR* presentInfo);
}
