%~dp0/../Tool/glslc.exe -fshader-stage=comp --target-env=vulkan1.3 Source/VolumetricCloudRayMarching.glsl -O -o Spirv/VolumetricCloudRayMarching.comp.spv
%~dp0/../Tool/glslc.exe -fshader-stage=comp --target-env=vulkan1.3 Source/VolumetricCompositeWithScreen.glsl -O -o Spirv/VolumetricCompositeWithScreen.comp.spv
%~dp0/../Tool/glslc.exe -fshader-stage=comp --target-env=vulkan1.3 Source/VolumetricCloudNoiseBasic.glsl -O -o Spirv/VolumetricCloudNoiseBasic.comp.spv
%~dp0/../Tool/glslc.exe -fshader-stage=comp --target-env=vulkan1.3 Source/VolumetricCloudNoiseWorley.glsl -O -o Spirv/VolumetricCloudNoiseWorley.comp.spv