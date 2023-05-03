#include "../renderer_interface.h"
#include "../render_scene.h"
#include "../renderer.h"
#include "../scene_textures.h"

namespace engine
{
    class CloudPass : public PassInterface
    {
    public:
        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

        std::unique_ptr<ComputePipeResources> computeCloudPipeline;
        std::unique_ptr<ComputePipeResources> reconstructionPipeline;
        std::unique_ptr<ComputePipeResources> compositeCloudPipeline;

    public:
        virtual void onInit() override
        {
            // Config code.
            getContext()->descriptorFactoryBegin()
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  kCommonShaderStage, 0) // imageHdrSceneColor
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 1) // inHdrSceneColor
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  kCommonShaderStage, 2) // imageCloudRenderTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 3) // inCloudRenderTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 4) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 5) // inGBufferA
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 6) // inBasicNoise
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 7) // inDetailNoise
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 8) // inCloudWeather
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 9) // inCloudCurl
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 10) // inTransmittanceLut
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 11) // inFroxelScatter
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  kCommonShaderStage, 12) // imageCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 13) // inCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  kCommonShaderStage, 14) // imageCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 15) // inCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  kCommonShaderStage, 16) // imageCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 17) // inCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 18) // inCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 19) // inCloudReconstructionTexture
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  kCommonShaderStage, 20) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kCommonShaderStage, 21) // Framedata.
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kCommonShaderStage, 22) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kCommonShaderStage, 23) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kCommonShaderStage, 24) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kCommonShaderStage, 25) // inDepth
                .bindNoInfo(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kCommonShaderStage, 26) // inDepth
                .buildNoInfoPush(setLayout);


            std::vector<VkDescriptorSetLayout> setLayouts = 
            { 
                  setLayout
                , m_context->getSamplerCache().getCommonDescriptorSetLayout()
                , getRenderer()->getBlueNoise().spp_1_buffer.setLayouts
            };

            computeCloudPipeline   = std::make_unique<ComputePipeResources>("shader/cloud_raymarching.comp.spv", 0, setLayouts);
            reconstructionPipeline = std::make_unique<ComputePipeResources>("shader/cloud_reconstruct.comp.spv", 0, setLayouts);
            compositeCloudPipeline = std::make_unique<ComputePipeResources>("shader/cloud_composite.comp.spv",   0, setLayouts);
        }

        virtual void release() override
        {
            computeCloudPipeline.reset();
            reconstructionPipeline.reset();
            compositeCloudPipeline.reset();
        }
    };

    void RendererInterface::renderVolumetricCloud(VkCommandBuffer cmd, GBufferTextures* inGBuffers, RenderScene* scene, BufferParameterHandle perFrameGPU, AtmosphereTextures& inAtmosphere)
    {
        if (!scene->isSkyExist())
        {
            return;
        }

        auto* pass = getContext()->getPasses().get<CloudPass>();
        auto* rtPool = &m_context->getRenderTargetPools();


        auto& sceneDepthZ = inGBuffers->depthTexture->getImage();
        auto& sceneColorHdr = inGBuffers->hdrSceneColor->getImage();
        auto& gbufferA = inGBuffers->gbufferA->getImage();

        // Quater resolution evaluate.
        auto computeCloud = rtPool->createPoolImage(
            "CloudCompute",
            sceneDepthZ.getExtent().width / 4,
            sceneDepthZ.getExtent().height / 4,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
        auto computeFog = rtPool->createPoolImage(
            "CloudFogCompute",
            sceneDepthZ.getExtent().width / 4,
            sceneDepthZ.getExtent().height / 4,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
        auto computeCloudDepth = rtPool->createPoolImage(
            "CloudComputeDepth",
            sceneDepthZ.getExtent().width / 4,
            sceneDepthZ.getExtent().height / 4,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );

        inAtmosphere.transmittance->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        inAtmosphere.skyView->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        inAtmosphere.froxelScatter->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());

        auto newCloudReconstruction = rtPool->createPoolImage(
            "NewCloudReconstruction",
            sceneDepthZ.getExtent().width,
            sceneDepthZ.getExtent().height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);


        auto newCloudFogReconstruction = rtPool->createPoolImage(
            "NewCloudFogReconstruction",
            sceneDepthZ.getExtent().width,
            sceneDepthZ.getExtent().height,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        auto newCloudReconstructionDepth = rtPool->createPoolImage(
            "NewCloudReconstructionDepth",
            sceneDepthZ.getExtent().width,
            sceneDepthZ.getExtent().height,
            VK_FORMAT_R32_SFLOAT,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        if (!m_cloudReconstruction)
        {
            m_cloudReconstruction = rtPool->createPoolImage(
                "CloudReconstruction",
                sceneDepthZ.getExtent().width,
                sceneDepthZ.getExtent().height,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            m_cloudReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }
        if (!m_cloudFogReconstruction)
        {
            m_cloudFogReconstruction = rtPool->createPoolImage(
                "CloudFogReconstruction",
                sceneDepthZ.getExtent().width,
                sceneDepthZ.getExtent().height,
                VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            m_cloudFogReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }
        if (!m_cloudReconstructionDepth)
        {
            m_cloudReconstructionDepth = rtPool->createPoolImage(
                "CloudReconstructionDepth",
                sceneDepthZ.getExtent().width,
                sceneDepthZ.getExtent().height,
                VK_FORMAT_R32_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            m_cloudReconstructionDepth->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }

        auto weatherTexture = std::dynamic_pointer_cast<GPUImageAsset>(getContext()->getEngineAsset(EBuiltinEngineAsset::Texture_CloudWeather));
        auto curlNoiseTexture = std::dynamic_pointer_cast<GPUImageAsset>(getContext()->getEngineAsset(EBuiltinEngineAsset::Texture_Noise));

        PushSetBuilder setBuilder(cmd);
        setBuilder
            .addUAV(sceneColorHdr)
            .addSRV(sceneColorHdr)
            .addUAV(computeCloud)
            .addSRV(computeCloud)
            .addSRV(sceneDepthZ, RHIDefaultImageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT))
            .addSRV(gbufferA)
            .addSRV(*m_renderer->getSharedTextures().cloudBasicNoise, buildBasicImageSubresource(), VK_IMAGE_VIEW_TYPE_3D)
            .addSRV(*m_renderer->getSharedTextures().cloudDetailNoise, buildBasicImageSubresource(), VK_IMAGE_VIEW_TYPE_3D)
            .addSRV(weatherTexture->getImage()) // 8
            .addSRV(curlNoiseTexture->getImage()) // 8
            .addSRV(inAtmosphere.transmittance) // 10
            .addSRV(inAtmosphere.froxelScatter, buildBasicImageSubresource(), VK_IMAGE_VIEW_TYPE_3D) // 11
            .addUAV(newCloudReconstruction) // 12
            .addSRV(newCloudReconstruction) // 13
            .addUAV(computeCloudDepth) // 14
            .addSRV(computeCloudDepth) // 15
            .addUAV(newCloudReconstructionDepth) // 16
            .addSRV(newCloudReconstructionDepth) // 17
            .addSRV(m_cloudReconstruction) // 18
            .addSRV(m_cloudReconstructionDepth) // 19
            .addSRV(inAtmosphere.skyView) // 20
            .addBuffer(perFrameGPU) // 21
            .addUAV(computeFog)
            .addSRV(computeFog)
            .addUAV(newCloudFogReconstruction)
            .addSRV(newCloudFogReconstruction)
            .addSRV(m_cloudFogReconstruction)
            .push(pass->computeCloudPipeline.get());

        std::vector<VkDescriptorSet> additionalSets =
        {
            m_context->getSamplerCache().getCommonDescriptorSet(),
            getRenderer()->getBlueNoise().spp_1_buffer.set
        };
        pass->computeCloudPipeline->bindSet(cmd, additionalSets, 1);


        {
            ScopePerframeMarker marker(cmd, "Compute Cloud", { 1.0f, 1.0f, 0.0f, 1.0f });

            computeCloud->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
            computeFog->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
            computeCloudDepth->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());

            pass->computeCloudPipeline->bind(cmd);
            vkCmdDispatch(cmd,
                getGroupCount(computeCloud->getImage().getExtent().width, 8),
                getGroupCount(computeCloud->getImage().getExtent().height, 8), 1);

            computeCloud->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
            computeFog->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
            computeCloudDepth->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }

        {
            ScopePerframeMarker marker(cmd, "Compute Reconstruction", { 1.0f, 1.0f, 0.0f, 1.0f });
            newCloudReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
            newCloudReconstructionDepth->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
            newCloudFogReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());
            pass->reconstructionPipeline->bind(cmd);

            vkCmdDispatch(cmd,
                getGroupCount(newCloudReconstruction->getImage().getExtent().width, 8),
                getGroupCount(newCloudReconstruction->getImage().getExtent().height, 8), 1);
            newCloudFogReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
            newCloudReconstruction->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
            newCloudReconstructionDepth->getImage().transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }

        {
            ScopePerframeMarker marker(cmd, "Compute Composite", { 1.0f, 1.0f, 0.0f, 1.0f });
            sceneColorHdr.transitionLayout(cmd, VK_IMAGE_LAYOUT_GENERAL, buildBasicImageSubresource());

            pass->compositeCloudPipeline->bind(cmd);

            vkCmdDispatch(cmd, getGroupCount(sceneColorHdr.getExtent().width, 8), getGroupCount(sceneColorHdr.getExtent().height, 8), 1);
            sceneColorHdr.transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, buildBasicImageSubresource());
        }

        m_cloudReconstruction = newCloudReconstruction;
        m_cloudReconstructionDepth = newCloudReconstructionDepth;
        m_cloudFogReconstruction = newCloudFogReconstruction;

        m_gpuTimer.getTimeStamp(cmd, "Volumetric Cloud");
    }
}