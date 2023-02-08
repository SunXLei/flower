#pragma once
#include "Scene.h"
#include "SceneNode.h"
#include "Component/SunSky.h"
#include "Component/Landscape.h"
#include "Component/Light.h"
#include "Component/StaticMesh.h"
#include "Component/PMXComponent.h"
#include "Component/SpotLight.h"
#include "Component/Transform.h"
#include "Component/PostprocessingVolume.h"
#include "Component/ReflectionCapture.h"

#include "../Version.h"

#define MAKE_VERSION_SCENE(ClassType) CEREAL_CLASS_VERSION(ClassType, SCENE_VERSION_CONTROL)

#define SCENE_ARCHIVE_IMPL_BASIC(CompNameXX) \
MAKE_VERSION_SCENE(Flower::CompNameXX); \
CEREAL_REGISTER_TYPE_WITH_NAME(Flower::CompNameXX, "Flower::"#CompNameXX);

#define SCENE_ARCHIVE_IMPL(CompNameXX) \
SCENE_ARCHIVE_IMPL_BASIC(CompNameXX); \
template<class Archive> \
void Flower::CompNameXX::serialize(Archive& archive, std::uint32_t const version)

#define SCENE_ARCHIVE_IMPL_INHERIT(CompNameXX, CompNamePP) \
SCENE_ARCHIVE_IMPL_BASIC(CompNameXX); \
CEREAL_REGISTER_POLYMORPHIC_RELATION(Flower::CompNamePP, Flower::CompNameXX)\
template<class Archive> \
void Flower::CompNameXX::serialize(Archive& archive, std::uint32_t const version){ \
archive(cereal::base_class<Flower::CompNamePP>(this));

#define SCENE_ARCHIVE_IMPL_INHERIT_END }


// Scene archive start.
///////////////////////////////////////////////////////////////////////////////////

SCENE_ARCHIVE_IMPL(Component)
{
	archive(m_node);
}

SCENE_ARCHIVE_IMPL_INHERIT(Transform, Component)
{
	archive(m_translation);
	archive(m_rotation);
	archive(m_scale);
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(LightComponent, Component)
{
	archive(m_color);
	archive(m_forward);
	archive(m_intensity);
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(SpotLightComponent, LightComponent)
{
	archive(bCastShadow);
	archive(innerCone);
	archive(outerCone);
	archive(range);
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(SunSkyComponent, LightComponent)
{
	if(version <= 8)
	{
		return;
	}
	archive(m_percascadeDimXY);
	archive(m_cascadeCount);
	archive(m_shadowFilterSize);
	archive(m_maxFilterSize);
	archive(m_cascadeSplitLambda);
	archive(m_shadowBiasConst);
	archive(m_shadowBiasSlope);
	archive(m_cascadeBorderAdopt);
	archive(m_cascadeEdgeLerpThreshold);
	archive(m_maxDrawDepthDistance);

	if(version > 8)
	{
		archive(m_earthAtmosphere.absorptionColor);
		archive(m_earthAtmosphere.absorptionLength);
		archive(m_earthAtmosphere.rayleighScatteringColor);
		archive(m_earthAtmosphere.rayleighScatterLength);

		archive(m_earthAtmosphere.multipleScatteringFactor);
		archive(m_earthAtmosphere.miePhaseFunctionG);
		archive(m_earthAtmosphere.bottomRadius);
		archive(m_earthAtmosphere.topRadius);

		archive(m_earthAtmosphere.mieScatteringColor);
		archive(m_earthAtmosphere.mieScatteringLength);
		archive(m_earthAtmosphere.mieAbsColor);
		archive(m_earthAtmosphere.mieAbsLength);

		archive(m_earthAtmosphere.mieAbsorption);
		archive(m_earthAtmosphere.viewRayMarchMinSPP);
		archive(m_earthAtmosphere.groundAlbedo);
		archive(m_earthAtmosphere.viewRayMarchMaxSPP);

		for(uint32_t i = 0; i < 12; i++)
		{
			archive(m_earthAtmosphere.rayleighDensity[i]);
			archive(m_earthAtmosphere.mieDensity[i]);
			archive(m_earthAtmosphere.absorptionDensity[i]);
		}
		


		archive(m_earthAtmosphere.cloudAreaStartHeight);
		archive(m_earthAtmosphere.cloudAreaThickness);
		archive(m_earthAtmosphere.atmospherePreExposure);
		archive(m_earthAtmosphere.cloudShadowExtent);

		archive(m_earthAtmosphere.cloudWeatherUVScale);
		archive(m_earthAtmosphere.cloudCoverage);
		archive(m_earthAtmosphere.cloudDensity);
		archive(m_earthAtmosphere.cloudShadingSunLightScale);

		archive(m_earthAtmosphere.cloudFogFade);
		archive(m_earthAtmosphere.cloudMaxTraceingDistance);
		archive(m_earthAtmosphere.cloudTracingStartMaxDistance);

		if (version > 9)
		{
			archive(m_earthAtmosphere.cloudLightStepMul);
			archive(m_earthAtmosphere.cloudLightBasicStep);
			archive(m_earthAtmosphere.cloudLightStepNum);
			archive(m_earthAtmosphere.cloudEnableGroundContribution);
		}
	}
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(StaticMeshComponent, Component)
{
	archive(m_staticMeshUUID);
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(LandscapeComponent, Component)
{

}
SCENE_ARCHIVE_IMPL_INHERIT_END



MAKE_VERSION_SCENE(Flower::PMXDrawMaterial);
template<class Archive>
void Flower::PMXDrawMaterial::serialize(Archive& archive, std::uint32_t const version)
{
	archive(material.m_enName);
	archive(material.m_name);

	archive(material.m_diffuse);
	archive(material.m_alpha);
	archive(material.m_specular);
	archive(material.m_specularPower);
	archive(material.m_ambient);
	archive(material.m_edgeFlag);
	archive(material.m_edgeSize);
	archive(material.m_edgeColor);
	archive(material.m_texture);

	archive(material.m_spTexture);

	uint32_t spTextureMode = uint32_t(material.m_spTextureMode);
	archive(spTextureMode); //
	material.m_spTextureMode = saba::MMDMaterial::SphereTextureMode(spTextureMode);

	archive(material.m_toonTexture);
	archive(material.m_textureMulFactor);
	archive(material.m_spTextureMulFactor);

	archive(material.m_toonTextureMulFactor);
	archive(material.m_textureAddFactor);
	archive(material.m_spTextureAddFactor);
	archive(material.m_toonTextureAddFactor);

	archive(material.m_bothFace);
	archive(material.m_groundShadow);
	archive(material.m_shadowCaster);
	archive(material.m_shadowReceiver);

	archive(bTranslucent);
	archive(bHide);

	if (version > 2)
	{
		archive(pixelDepthOffset);
	}

	if (version > 7)
	{
		archive(pmxShadingModel);
	}
}

SCENE_ARCHIVE_IMPL_INHERIT(PMXComponent, Component)
{
	archive(m_pmxPath);
	archive(m_vmdPath);
	archive(m_wavPath);
	archive(m_cameraPath);

	if (version > 1)
	{
		archive(m_materials);
	}
	
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(ReflectionCaptureComponent, Component)
{
	archive(m_bUseIBLTexture);

	if (version > 0)
	{
		archive(m_radius);
	}
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL_INHERIT(PostprocessVolumeComponent, Component)
{
	archive(m_settings.bloomIntensity);
	archive(m_settings.bloomRadius);
	archive(m_settings.bloomThreshold);
	archive(m_settings.bloomThresholdSoft);

	archive(m_settings.autoExposureLowPercent);
	archive(m_settings.autoExposureHighPercent);
	archive(m_settings.autoExposureMinBrightness);
	archive(m_settings.autoExposureMaxBrightness);
	archive(m_settings.autoExposureSpeedDown);
	archive(m_settings.autoExposureSpeedUp);
	archive(m_settings.autoExposureExposureCompensation);

	archive(m_settings.gtaoSliceNum);
	archive(m_settings.gtaoStepNum);
	archive(m_settings.gtaoRadius);
	archive(m_settings.gtaoThickness);
	archive(m_settings.gtaoPower);
	archive(m_settings.gtaoIntensity);

	archive(m_settings.tonemapper_P);
	archive(m_settings.tonemapper_a);
	archive(m_settings.tonemapper_m);
	archive(m_settings.tonemapper_l);
	archive(m_settings.tonemapper_c);
	archive(m_settings.tonemapper_b);
	archive(m_settings.tonemmaper_s);
	archive(m_settings.saturation);

	if (version > 3)
	{
		archive(m_settings.bDofEnable);
		archive(m_settings.dof_focusDistance);
		archive(m_settings.dof_aperture);
		archive(m_settings.dof_bUseCameraFOV);
		archive(m_settings.dof_focusLength);
		archive(m_settings.dof_kernelSize);
		archive(m_settings.dof_FilmHeight);
		if (version > 4)
		{
			archive(m_settings.dof_focusMode);
			archive(m_settings.dof_focusPoint);
		}
		if (version > 5)
		{
			archive(m_settings.vignette_falloff);
		}
		if (version > 6)
		{
			archive(m_settings.bEnableVignette);
			archive(m_settings.bEnableFringeMode);

			archive(m_settings.fringe_barrelStrength);
			archive(m_settings.fringe_zoomStrength);
			archive(m_settings.fringe_lateralShift);
			archive(m_settings.dof_bNearBlur);
		}
		if (version > 7)
		{
			archive(m_settings.dof_trackPMXMode);
		}
		if (version > 8)
		{
			archive(m_settings.dof_pmxFoucusMinOffset);
		}
	}
}
SCENE_ARCHIVE_IMPL_INHERIT_END

SCENE_ARCHIVE_IMPL(SceneNode)
{
	archive(m_bVisibility);
	archive(m_bStatic);
	archive(m_id);
	archive(m_runTimeIdName);
	archive(m_depth);
	archive(m_name);
	archive(m_parent);
	archive(m_scene);
	archive(m_components);
	archive(m_children);
}

SCENE_ARCHIVE_IMPL(Scene)
{
	archive(m_currentId);
	archive(m_root);
	archive(m_initName);
	archive(m_cacheSceneComponents);
	archive(m_nodeCount);
	archive(m_savePath);
}